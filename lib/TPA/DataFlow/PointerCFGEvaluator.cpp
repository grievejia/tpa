#include "MemoryModel/Precision/KLimitContext.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"
#include "TPA/DataFlow/PointerCFGEvaluator.h"

#include "PointerAnalysis/ControlFlow/PointerCFGNodePrint.h"
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

PointerCFGEvaluator::PointerCFGEvaluator(const Context* c, const PointerCFG* p, GlobalStateType& gs, GlobalWorkListType& wl, TransferFunction<PointerCFG>& tf): ctx(c), cfg(p), globalState(gs), globalWorkList(wl), localWorkList(globalWorkList.getLocalWorkList(ctx, cfg)), transferFunction(tf)
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
	auto envChanged = transferFunction.evalAlloc(ctx, allocNode, globalState.getEnv());

	if (envChanged)
		propagateTopLevel(allocNode);
}

void PointerCFGEvaluator::visitCopyNode(const CopyNode* copyNode)
{
	bool isValid;
	bool envChanged;
	std::tie(isValid, envChanged) = transferFunction.evalCopy(ctx, copyNode, globalState.getEnv());

	if (isValid && envChanged)
		propagateTopLevel(copyNode);
}

void PointerCFGEvaluator::visitLoadNode(const LoadNode* loadNode)
{
	auto store = globalState.getMemo().lookup(ctx, loadNode);
	if (store == nullptr)
		return;
	auto newStore = *store;

	bool isValid;
	bool envChanged;
	std::tie(isValid, envChanged) = transferFunction.evalLoad(ctx, loadNode, globalState.getEnv(), newStore);

	if (isValid)
	{
		if (envChanged)
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

	bool isValid;
	bool storeChanged;
	std::tie(isValid, storeChanged) = transferFunction.evalStore(ctx, storeNode, globalState.getEnv(), newStore);
	
	if (isValid)
		propagateMemLevel(storeNode, newStore);
}

void PointerCFGEvaluator::visitCallNode(const CallNode* callNode)
{
	auto store = globalState.getMemo().lookup(ctx, callNode);
	if (store == nullptr)
		return;

	auto callees = transferFunction.resolveCallTarget(ctx, callNode, globalState.getEnv(), globalState.getProgram().at_funs());

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

void PointerCFGEvaluator::applyCall(const CallNode* callNode, const Function* callee, const Store& store)
{
	// Update call graph first
	auto newCtx = KLimitContext::pushContext(ctx, callNode->getInstruction(), callee);
	globalState.getCallGraph().insertCallEdge(std::make_pair(ctx, callNode->getInstruction()), std::make_pair(newCtx, callee));

	// Handle external function call here
	if (callee->isDeclaration() || callee->isIntrinsic())
	{
		// Ask transferFunction compute the side effect of callee
		bool isValid, envChanged;
		auto newStore = store;
		std::tie(isValid, envChanged, std::ignore) = transferFunction.applyExternal(ctx, callNode, globalState.getEnv(), newStore, callee);
		if (!isValid)
			return;

		// There are some special cases where there is no need to keep propagating info. e.g. calling exit() or abort().
		if (callee->getName() == "exit" || callee->getName() == "_exit" || callee->getName() == "abort")
			return;

		// External calls are treated as language primitives. There is no need to redirect control flow to another function. Hence we just need to progress as usual
		if (envChanged)
			propagateTopLevel(callNode);
		propagateMemLevel(callNode, newStore);
		return;
	}

	auto tgtCFG = globalState.getProgram().getPointerCFG(callee);
	assert(tgtCFG != nullptr);
	auto tgtEntryNode = tgtCFG->getEntryNode();

	bool isValid, envChanged;
	std::tie(isValid, envChanged) = transferFunction.evalCall(ctx, callNode, newCtx, *tgtCFG, globalState.getEnv());
	if (!isValid)
		return;

	auto storeChanged = globalState.getMemo().updateMemo(newCtx, tgtEntryNode, store);
	// The reason why we could put envChanged into the if condition is we know that the only change in env that may affect the callee are the bindings for global values, yet those bindings won't change. 
	if (envChanged || storeChanged)
		globalWorkList.enqueue(newCtx, tgtCFG, tgtEntryNode);

	// Prevent premature fixpoint
	// We enqueue the callee's return node rather than caller's successors of the call node. The reason is that if the callee reaches its fixpoint, the call node's lhs won't get updated
	// FIXME: should only propagate once for each non-external callsite
	globalWorkList.enqueue(newCtx, tgtCFG, tgtCFG->getExitNode());
}

void PointerCFGEvaluator::applyReturn(const ReturnNode* retNode, const Context* oldCtx, const Instruction* oldInst, const Store& store)
{
	auto oldCFG = globalState.getProgram().getPointerCFG(oldInst->getParent()->getParent());
	assert(oldCFG != nullptr);
	auto callSiteNode = oldCFG->getNodeFromInstruction(oldInst);
	assert(callSiteNode != nullptr);

	bool isValid, envChanged;
	std::tie(isValid, envChanged) = transferFunction.evalReturn(ctx, retNode, oldCtx, cast<CallNode>(callSiteNode), globalState.getEnv());
	if (!isValid)
		return;

	for (auto succ: callSiteNode->succs())
	{
		auto storeChanged = globalState.getMemo().updateMemo(oldCtx, succ, store);
		if (storeChanged)
			globalWorkList.enqueue(oldCtx, oldCFG, succ);
	}
	if (envChanged)
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
