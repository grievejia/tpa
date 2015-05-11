#include "MemoryModel/Precision/KLimitContext.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"
#include "TPA/DataFlow/PointerCFGEvaluator.h"
#include "TPA/DataFlow/TransferFunction.h"

#include "PointerAnalysis/ControlFlow/PointerCFGNodePrint.h"

using namespace llvm;

namespace tpa
{

PointerCFGEvaluator::PointerCFGEvaluator(const Context* c, const PointerCFG* p, SemiSparseGlobalState& gs, GlobalWorkListType& wl): ctx(c), cfg(p), globalState(gs), globalWorkList(wl), localWorkList(globalWorkList.getLocalWorkList(ctx, cfg))
{
}

void PointerCFGEvaluator::propagateTopLevel(const PointerCFGNode* node)
{
	for (auto succ: node->uses())
		localWorkList.enqueue(succ);
}

void PointerCFGEvaluator::propagateMemLevel(const PointerCFGNode* node, const Store& store)
{
	for (auto succ: node->succs())
	{
		auto storeChanged = globalState.getMemo().updateMemo(ctx, succ, store);
		if (storeChanged)
			localWorkList.enqueue(succ);
	}
}

void PointerCFGEvaluator::visitEntryNode(const EntryNode* entryNode)
{
	auto store = globalState.getMemo().lookup(ctx, entryNode);
	assert(store != nullptr);

	propagateTopLevel(entryNode);
	propagateMemLevel(entryNode, *store);
}

void PointerCFGEvaluator::visitAllocNode(const AllocNode* allocNode)
{
	auto evalResult = TransferFunction(ctx, globalState).evalAllocNode(allocNode);

	if (evalResult.isValid() && evalResult.hasEnvChanged())
		propagateTopLevel(allocNode);
}

void PointerCFGEvaluator::visitCopyNode(const CopyNode* copyNode)
{
	auto evalResult = TransferFunction(ctx, globalState).evalCopyNode(copyNode);

	if (evalResult.isValid() && evalResult.hasEnvChanged())
		propagateTopLevel(copyNode);
}

void PointerCFGEvaluator::visitLoadNode(const LoadNode* loadNode)
{
	auto store = globalState.getMemo().lookup(ctx, loadNode);
	if (store == nullptr)
		return;
	auto newStore = *store;

	auto evalResult = TransferFunction(ctx, newStore, globalState).evalLoadNode(loadNode);

	if (evalResult.isValid())
	{
		if (evalResult.hasEnvChanged())
			propagateTopLevel(loadNode);
		propagateMemLevel(loadNode, newStore);
	}
}

void PointerCFGEvaluator::visitStoreNode(const StoreNode* storeNode)
{
	auto store = globalState.getMemo().lookup(ctx, storeNode);
	if (store == nullptr)
		return;
	auto newStore = *store;

	auto evalResult = TransferFunction(ctx, newStore, globalState).evalStoreNode(storeNode);
	if (evalResult.isValid())
		propagateMemLevel(storeNode, newStore);
}

void PointerCFGEvaluator::visitCallNode(const CallNode* callNode)
{
	auto store = globalState.getMemo().lookup(ctx, callNode);
	if (store == nullptr)
		return;

	auto callees = TransferFunction(ctx, globalState).resolveCallTarget(callNode);

	for (auto f: callees)
		applyCall(callNode, f, *store);
}

void PointerCFGEvaluator::visitReturnNode(const ReturnNode* retNode)
{
	if (cfg == globalState.getProgram().getEntryCFG())
	{
		// Return from main. Do nothing
		errs() << "Reached program end\n";
		return;
	}

	auto store = globalState.getMemo().lookup(ctx, retNode);
	if (store == nullptr)
		return;

	for (auto retSite: globalState.getCallGraph().getCallSites(std::make_pair(ctx, cfg->getFunction())))
	{
		auto oldCtx = retSite.first;
		auto oldInst = retSite.second;
		
		applyReturn(retNode, oldCtx, oldInst, *store);
	}
}

void PointerCFGEvaluator::applyExternalCall(const CallNode* callNode, const Context* newCtx, const llvm::Function* callee, const Store& store)
{
	// There are some special cases where there is no need to keep propagating info. e.g. calling exit() or abort().
	if (callee->getName() == "exit" || callee->getName() == "_exit" || callee->getName() == "abort")
		return;

	// Ask transferFunction to compute the side effect of callee
	auto newStore = store;

	auto evalResult = TransferFunction(ctx, newStore, globalState).evalExternalCall(callNode, callee);

	if (!evalResult.isValid())
		return;

	// External calls are treated as language primitives. There is no need to redirect control flow to another function. Hence we just need to progress as usual
	if (evalResult.hasEnvChanged())
		propagateTopLevel(callNode);
	propagateMemLevel(callNode, newStore);
}

void PointerCFGEvaluator::applyNonExternalCall(const CallNode* callNode, const Context* newCtx, const llvm::Function* callee, const Store& store)
{
	auto tgtCFG = globalState.getProgram().getPointerCFG(callee);
	assert(tgtCFG != nullptr);
	auto tgtEntryNode = tgtCFG->getEntryNode();

	auto evalResult = TransferFunction(ctx, globalState).evalCallArguments(callNode, newCtx, callee);
	if (!evalResult.isValid())
		return;

	auto storeChanged = globalState.getMemo().updateMemo(newCtx, tgtEntryNode, store);
	// The reason why we could put envChanged into the if condition is we know that the only change in env that may affect the callee are the bindings for global values, yet those bindings won't change. 
	if (evalResult.hasEnvChanged() || storeChanged)
		globalWorkList.enqueue(newCtx, tgtCFG, tgtEntryNode);

	// Prevent premature fixpoint
	// We enqueue the callee's return node rather than caller's successors of the call node. The reason is that if the callee reaches its fixpoint, the call node's lhs won't get updated
	// FIXME: should only propagate once for each non-external callsite
	if (!tgtCFG->doesNotReturn())
		globalWorkList.enqueue(newCtx, tgtCFG, tgtCFG->getExitNode());
}

void PointerCFGEvaluator::applyCall(const CallNode* callNode, const Function* callee, const Store& store)
{
	// Update call graph first
	auto newCtx = KLimitContext::pushContext(ctx, callNode->getInstruction(), callee);
	globalState.getCallGraph().insertCallEdge(std::make_pair(ctx, callNode->getInstruction()), std::make_pair(newCtx, callee));

	// Handle external function call here
	if (callee->isDeclaration())
		applyExternalCall(callNode, newCtx, callee, store);
	else
		applyNonExternalCall(callNode, newCtx, callee, store);
}

void PointerCFGEvaluator::applyReturn(const ReturnNode* retNode, const Context* oldCtx, const Instruction* oldInst, const Store& store)
{
	auto oldCFG = globalState.getProgram().getPointerCFG(oldInst->getParent()->getParent());
	assert(oldCFG != nullptr);
	auto callSiteNode = oldCFG->getNodeFromInstruction(oldInst);
	assert(callSiteNode != nullptr);

	auto evalResult = TransferFunction(ctx, globalState).evalReturnValue(retNode, oldCtx, cast<CallNode>(callSiteNode));
	if (!evalResult.isValid())
		return;

	for (auto succ: callSiteNode->succs())
	{
		auto storeChanged = globalState.getMemo().updateMemo(oldCtx, succ, store);
		if (storeChanged)
			globalWorkList.enqueue(oldCtx, oldCFG, succ);
	}
	if (evalResult.hasEnvChanged())
		for (auto succ: callSiteNode->uses())
			globalWorkList.enqueue(oldCtx, oldCFG, succ);
}

void PointerCFGEvaluator::eval()
{
	//errs() << "<Function " << cfg->getFunction()->getName() << ">\n";
	while (!localWorkList.isEmpty())
	{
		auto node = localWorkList.dequeue();
		//errs() << "node = " << *node << "\n";

		visit(node);
	}
}

}
