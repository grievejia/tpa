#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "MemoryModel/PtsSet/Env.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"
#include "TPA/DataFlow/StaticCallGraph.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "MemoryModel/Precision/KLimitContext.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

void PointerAnalysisEngine::propagateTopLevel(const PointerCFGNode* node, LocalWorkList& workList)
{
	for (auto succ: node->uses())
		workList.enqueue(succ);
}

void PointerAnalysisEngine::propagateMemoryLevel(const Context* ctx, const PointerCFGNode* node, const Store& store, LocalWorkList& workList)
{
	for (auto succ: node->succs())
	{
		if (memo.updateMemo(ctx, succ, store))
			workList.enqueue(succ);
	}
}

void PointerAnalysisEngine::applyFunction(const Context* ctx, const CallNode* callNode, const Function* callee, const PointerProgram& prog, Env& env, Store store, EvaluationWorkList& funWorkList, LocalWorkList& workList)
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

	auto tgtCFG = prog.getPointerCFG(callee);
	assert(tgtCFG != nullptr);

	bool isValid, envChanged;
	std::tie(isValid, envChanged) = transferFunction.evalCall(ctx, callNode, newCtx, *tgtCFG, env);
	if (!isValid)
		return;

	auto storeChanged = memo.updateMemo(newCtx, tgtCFG->getEntryNode(), store);
	if (envChanged || storeChanged)
		funWorkList.enqueue(newCtx, tgtCFG, tgtCFG->getEntryNode());
	propagateMemoryLevel(ctx, callNode, store, workList);
}

void PointerAnalysisEngine::evalFunction(const Context* ctx, const PointerCFG* cfg, Env& env, EvaluationWorkList& funWorkList, const PointerProgram& prog)
{
	auto& workList = funWorkList.getLocalWorkList(ctx, cfg);

	//errs() << "<Function " << cfg->getFunction()->getName() << ">\n";
	while (!workList.isEmpty())
	{
		auto node = workList.dequeue();
		
		//errs() << "node = " << node->toString() << "\n";

		switch (node->getType())
		{
			case PointerCFGNodeType::Entry:
			{
				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				propagateTopLevel(node, workList);
				propagateMemoryLevel(ctx, node, store, workList);
				break;
			}
			case PointerCFGNodeType::Alloc:
			{
				auto allocNode = cast<AllocNode>(node);

				auto envChanged = transferFunction.evalAlloc(ctx, allocNode, env);

				if (envChanged)
					propagateTopLevel(allocNode, workList);

				break;
			}
			case PointerCFGNodeType::Copy:
			{
				auto copyNode = cast<CopyNode>(node);

				bool isValid;
				bool envChanged;
				std::tie(isValid, envChanged) = transferFunction.evalCopy(ctx, copyNode, env);

				if (isValid && envChanged)
					propagateTopLevel(copyNode, workList);

				break;
			}
			case PointerCFGNodeType::Load:
			{
				auto loadNode = cast<LoadNode>(node);

				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				bool isValid;
				bool envChanged;
				std::tie(isValid, envChanged) = transferFunction.evalLoad(ctx, loadNode, env, store);

				if (isValid)
				{
					if (envChanged)
						propagateTopLevel(loadNode, workList);
					propagateMemoryLevel(ctx, loadNode, store, workList);
				}

				break;
			}
			case PointerCFGNodeType::Store:
			{
				auto storeNode = cast<StoreNode>(node);
				
				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				bool isValid;
				bool storeChanged;
				std::tie(isValid, storeChanged) = transferFunction.evalStore(ctx, storeNode, env, store);
				
				if (isValid)
					propagateMemoryLevel(ctx, storeNode, store, workList);
				break;
			}
			case PointerCFGNodeType::Call:
			{
				auto callNode = cast<CallNode>(node);
				
				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				auto callees = transferFunction.resolveCallTarget(ctx, callNode, env, prog);

				for (auto f: callees)
					applyFunction(ctx, callNode, f, prog, env, store, funWorkList, workList);

				break;
			}
			case PointerCFGNodeType::Ret:
			{
				auto retNode = cast<ReturnNode>(node);

				if (cfg == prog.getEntryCFG())
				{
					// Return from main. Do nothing
					errs() << "Reached program end\n";
					break;
				}

				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				for (auto retSite: callGraph.getCallSites(std::make_pair(ctx, cfg->getFunction())))
				{
					auto& oldCtx = retSite.first;
					auto& callInst = retSite.second;

					auto oldCFG = prog.getPointerCFG(callInst->getParent()->getParent());
					assert(oldCFG != nullptr);
					auto callNode = oldCFG->getNodeFromInstruction(callInst);
					assert(callNode != nullptr);

					bool isValid, envChanged;
					std::tie(isValid, envChanged) = transferFunction.evalReturn(ctx, retNode, oldCtx, cast<CallNode>(callNode), env);
					if (!isValid)
						break;

					for (auto succ: callNode->succs())
					{
						auto storeChanged = memo.updateMemo(oldCtx, succ, store);
						if (envChanged || storeChanged)
						{
							funWorkList.enqueue(oldCtx, oldCFG, succ);
						}
					}
					if (envChanged)
						for (auto succ: callNode->uses())
							funWorkList.enqueue(oldCtx, oldCFG, succ);
				}

				break;
			}
		}
	}
}

void PointerAnalysisEngine::runOnProgram(const PointerProgram& prog, Env& env, Store store)
{
	// The function-level worklist
	auto workList = EvaluationWorkList();

	// Initialize workList and memo
	auto entryCtx = Context::getGlobalContext();
	auto entryCFG = prog.getEntryCFG();
	auto entryNode = entryCFG->getEntryNode();
	memo.updateMemo(entryCtx, entryNode, store);
	workList.enqueue(entryCtx, entryCFG, entryNode);

	while (!workList.isEmpty())
	{
		const Context* ctx;
		const PointerCFG* cfg;
		std::tie(ctx, cfg) = workList.dequeue();

		evalFunction(ctx, cfg, env, workList, prog);
	}
}

}
