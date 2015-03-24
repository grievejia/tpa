#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "MemoryModel/Precision/KLimitContext.h"
#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"

#include "PointerAnalysis/ControlFlow/PointerCFGNodePrint.h"
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

PointerAnalysisEngine::PointerAnalysisEngine(PointerManager& p, MemoryManager& m, StoreManager& s, const PointerProgram& pp, Env& e, Store st, StaticCallGraph& g, Memo<PointerCFGNode>& me, const ExternalPointerEffectTable& t): prog(pp), callGraph(g), env(e), memo(me), transferFunction(p, m, s, t)
{
	initializeWorkList(std::move(st));
}

void PointerAnalysisEngine::initializeWorkList(Store store)
{
	// Initialize workList and memo
	auto entryCtx = Context::getGlobalContext();
	auto entryCFG = prog.getEntryCFG();
	auto entryNode = entryCFG->getEntryNode();
	memo.updateMemo(entryCtx, entryNode, store);
	globalWorkList.enqueue(entryCtx, entryCFG, entryNode);
}

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

void PointerAnalysisEngine::applyFunction(const Context* ctx, const CallNode* callNode, const Function* callee, Store store, LocalWorkList& localWorkList)
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
			propagateTopLevel(callNode, localWorkList);
		propagateMemoryLevel(ctx, callNode, store, localWorkList);
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
		globalWorkList.enqueue(newCtx, tgtCFG, tgtCFG->getEntryNode());
	// Prevent premature fixpoint
	// We enqueue the callee's return node rather than caller's successors of the call node. The reason is that if the callee reaches its fixpoint, the call node's lhs won't get updated
	// FIXME: should only propagate once for each non-external callsite
	globalWorkList.enqueue(newCtx, tgtCFG, tgtCFG->getExitNode());
}

void PointerAnalysisEngine::evalFunction(const Context* ctx, const PointerCFG* cfg)
{
	auto& localWorkList = globalWorkList.getLocalWorkList(ctx, cfg);

	//errs() << "<Function " << cfg->getFunction()->getName() << ">\n";
	while (!localWorkList.isEmpty())
	{
		auto node = localWorkList.dequeue();
		
		//errs() << "node = " << *node << "\n";

		switch (node->getType())
		{
			case PointerCFGNodeType::Entry:
			{
				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				propagateTopLevel(node, localWorkList);
				propagateMemoryLevel(ctx, node, store, localWorkList);
				break;
			}
			case PointerCFGNodeType::Alloc:
			{
				auto allocNode = cast<AllocNode>(node);

				auto envChanged = transferFunction.evalAlloc(ctx, allocNode, env);

				if (envChanged)
					propagateTopLevel(allocNode, localWorkList);

				break;
			}
			case PointerCFGNodeType::Copy:
			{
				auto copyNode = cast<CopyNode>(node);

				bool isValid;
				bool envChanged;
				std::tie(isValid, envChanged) = transferFunction.evalCopy(ctx, copyNode, env);

				if (isValid && envChanged)
					propagateTopLevel(copyNode, localWorkList);

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
						propagateTopLevel(loadNode, localWorkList);
					propagateMemoryLevel(ctx, loadNode, store, localWorkList);
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
					propagateMemoryLevel(ctx, storeNode, store, localWorkList);
				break;
			}
			case PointerCFGNodeType::Call:
			{
				auto callNode = cast<CallNode>(node);
				
				auto optStore = memo.lookup(ctx, node);
				if (!optStore)
					break;
				auto& store = *optStore;

				auto callees = transferFunction.resolveCallTarget(ctx, callNode, env, prog.at_funs());

				for (auto f: callees)
					applyFunction(ctx, callNode, f, store, localWorkList);

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
							globalWorkList.enqueue(oldCtx, oldCFG, succ);
						}
					}
					if (envChanged)
						for (auto succ: callNode->uses())
							globalWorkList.enqueue(oldCtx, oldCFG, succ);
				}

				break;
			}
		}
		//env.dump(errs());
	}
}

void PointerAnalysisEngine::run()
{
	while (!globalWorkList.isEmpty())
	{
		const Context* ctx;
		const PointerCFG* cfg;
		std::tie(ctx, cfg) = globalWorkList.dequeue();

		evalFunction(ctx, cfg);
	}
}

}
