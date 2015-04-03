#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "MemoryModel/Precision/KLimitContext.h"
#include "MemoryModel/PtsSet/Env.h"
#include "PointerAnalysis/DataFlow/DefUseProgram.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"
#include "TPA/DataFlow/SparseAnalysisEngine.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

void SparseAnalysisEngine::propagateTopLevel(const DefUseGraphNode* node, LocalWorkList& workList)
{
	for (auto succ: node->top_succs())
		workList.enqueue(succ);
}

void SparseAnalysisEngine::propagateMemoryLevel(const Context* ctx, const DefUseGraphNode* node, const Store& store, LocalWorkList& workList)
{
	for (auto const& mapping: node->mem_succs())
	{
		auto loc = mapping.first;
		auto pSet = store.lookup(loc);
		for (auto succ: mapping.second)
		{
			if (memo.updateMemo(ctx, succ, loc, pSet))
				workList.enqueue(succ);
		}
	}
}

Store SparseAnalysisEngine::pruneStore(const Store& store, const Function* f)
{
	auto retStore = Store();
	auto& summary = summaryMap.getSummary(f);
	for (auto loc: summary.mem_reads())
	{
		auto pSet = store.lookup(loc);
		if (!pSet.isEmpty())
			retStore.strongUpdate(loc, pSet);
	}
	for (auto loc: summary.mem_writes())
	{
		auto pSet = store.lookup(loc);
		if (!pSet.isEmpty())
			retStore.strongUpdate(loc, pSet);
	}
	return retStore;
}

void SparseAnalysisEngine::applyFunction(const Context* ctx, const CallDefUseNode* callNode, const llvm::Function* callee, const DefUseProgram& prog, Env& env, Store store, GlobalWorkList& funWorkList, LocalWorkList& workList)
{
	// Update call graph first
	auto newCtx = KLimitContext::pushContext(ctx, callNode->getInstruction(), callee);
	callGraph.insertCallEdge(std::make_pair(ctx, callNode->getInstruction()), std::make_pair(newCtx, callee));

	// Handle external function call here
	if (callee->isDeclaration() || callee->isIntrinsic())
	{
		// Ask transferFunction compute the side effect of callee
		bool isValid, envChanged;
		std::tie(isValid, envChanged, std::ignore) = transferFunction.applyExternal(ctx, callNode, env, store, callee);
		if (!isValid)
			return;

		// There are some special cases where there is no need to keep propagating info. e.g. calling exit() or abort().
		if (callee->getName() == "exit" || callee->getName() == "_exit" || callee->getName() == "abort")
			return;

		// External calls are treated as language primitives. There is no need to redirect control flow to another function. Hence we just need to progress as usual
		if (envChanged)
			propagateTopLevel(callNode, workList);
		propagateMemoryLevel(ctx, callNode, store, workList);
		return;
	}

	auto tgtCFG = prog.getDefUseGraph(callee);
	assert(tgtCFG != nullptr);

	bool isValid, envChanged;
	std::tie(isValid, envChanged) = transferFunction.evalCall(ctx, callNode, newCtx, *tgtCFG, env);
	if (!isValid)
		return;

	auto prunedStore = pruneStore(store, callee);
	auto storeChanged = memo.updateMemo(newCtx, tgtCFG->getEntryNode(), store);
	if (envChanged || storeChanged)
		funWorkList.enqueue(newCtx, tgtCFG, tgtCFG->getEntryNode());
	propagateMemoryLevel(ctx, callNode, store, workList);
}

void SparseAnalysisEngine::evalFunction(const Context* ctx, const DefUseGraph* dug, Env& env, GlobalWorkList& funWorkList, const DefUseProgram& prog)
{
	auto& workList = funWorkList.getLocalWorkList(ctx, dug);

	while (!workList.isEmpty())
	{
		auto node = workList.dequeue();

		switch (node->getType())
		{
			case PointerCFGNodeType::Entry:
			{
				auto store = memo.lookup(ctx, node);
				if (store == nullptr)
					break;

				propagateTopLevel(node, workList);
				propagateMemoryLevel(ctx, node, *store, workList);
				break;
			}
			case PointerCFGNodeType::Alloc:
			{
				auto allocNode = cast<AllocDefUseNode>(node);

				auto envChanged = transferFunction.evalAlloc(ctx, allocNode, env);

				if (envChanged)
					propagateTopLevel(allocNode, workList);

				break;
			}
			case PointerCFGNodeType::Copy:
			{
				auto copyNode = cast<CopyDefUseNode>(node);

				bool isValid;
				bool envChanged;
				std::tie(isValid, envChanged) = transferFunction.evalCopy(ctx, copyNode, env);

				if (isValid && envChanged)
					propagateTopLevel(copyNode, workList);

				break;
			}
			case PointerCFGNodeType::Load:
			{
				auto loadNode = cast<LoadDefUseNode>(node);

				auto store = memo.lookup(ctx, node);
				if (store == nullptr)
					break;
				auto newStore = *store;

				bool isValid;
				bool envChanged;
				std::tie(isValid, envChanged) = transferFunction.evalLoad(ctx, loadNode, env, newStore);

				if (isValid)
				{
					if (envChanged)
						propagateTopLevel(loadNode, workList);
					propagateMemoryLevel(ctx, loadNode, newStore, workList);
				}

				break;
			}
			case PointerCFGNodeType::Store:
			{
				auto storeNode = cast<StoreDefUseNode>(node);
				
				auto store = memo.lookup(ctx, node);
				if (store == nullptr)
					break;
				auto newStore = *store;

				bool isValid;
				bool storeChanged;
				std::tie(isValid, storeChanged) = transferFunction.evalStore(ctx, storeNode, env, newStore);
				
				if (isValid)
					propagateMemoryLevel(ctx, storeNode, newStore, workList);
				break;
			}
			case PointerCFGNodeType::Call:
			{
				auto callNode = cast<CallDefUseNode>(node);
				
				auto store = memo.lookup(ctx, node);
				if (store == nullptr)
					break;
				auto newStore = *store;

				auto callees = transferFunction.resolveCallTarget(ctx, callNode, env, prog.at_funs());

				for (auto f: callees)
					applyFunction(ctx, callNode, f, prog, env, newStore, funWorkList, workList);

				break;
			}
			case PointerCFGNodeType::Ret:
			{
				auto retNode = cast<ReturnDefUseNode>(node);

				if (dug == prog.getEntryGraph())
				{
					// Return from main. Do nothing
					errs() << "D Reached program end\n";
					break;
				}

				auto store = memo.lookup(ctx, node);
				if (store == nullptr)
					break;

				for (auto retSite: callGraph.getCallSites(std::make_pair(ctx, dug->getFunction())))
				{
					auto& oldCtx = retSite.first;
					auto& callInst = retSite.second;

					auto oldCFG = prog.getDefUseGraph(callInst->getParent()->getParent());
					assert(oldCFG != nullptr);
					auto callNode = oldCFG->getNodeFromInstruction(callInst);
					assert(callNode != nullptr);

					bool isValid, envChanged;
					std::tie(isValid, envChanged) = transferFunction.evalReturn(ctx, retNode, oldCtx, cast<CallDefUseNode>(callNode), env);
					if (!isValid)
						break;

					for (auto const& mapping: callNode->mem_succs())
					{
						auto loc = mapping.first;
						auto pSet = store->lookup(loc);
						if (!pSet.isEmpty())
						{
							for (auto succ: mapping.second)
							{
								if (memo.updateMemo(oldCtx, succ, loc, pSet))
									funWorkList.enqueue(oldCtx, oldCFG, succ);
							}
						}
					}
					if (envChanged)
						for (auto succ: callNode->top_succs())
							funWorkList.enqueue(oldCtx, oldCFG, succ);
				}

				break;
			}
		}
	}
}

void SparseAnalysisEngine::runOnDefUseProgram(const DefUseProgram& prog, Env& env, Store store)
{
	// The function-level worklist
	auto workList = GlobalWorkList();

	// Initialize workList and memo
	auto entryCtx = Context::getGlobalContext();
	auto entryGraph = prog.getEntryGraph();
	auto entryNode = entryGraph->getEntryNode();
	memo.updateMemo(entryCtx, entryNode, store);
	workList.enqueue(entryCtx, entryGraph, entryNode);

	while (!workList.isEmpty())
	{
		const Context* ctx;
		const DefUseGraph* dug;
		std::tie(ctx, dug) = workList.dequeue();

		evalFunction(ctx, dug, env, workList, prog);
	}
}

}