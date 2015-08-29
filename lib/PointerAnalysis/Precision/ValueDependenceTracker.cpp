#include "PointerAnalysis/Precision/ValueDependenceTracker.h"
#include "PointerAnalysis/Program/CFG/CFG.h"
#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "PointerAnalysis/Program/CFG/NodeVisitor.h"
#include "PointerAnalysis/Program/SemiSparseProgram.h"

#include <llvm/IR/Function.h>

using namespace context;
using namespace llvm;

namespace tpa
{

namespace
{

class ValueTracker: public ConstNodeVisitor<ValueTracker>
{
private:
	const SemiSparseProgram& ssProg;

	using CallGraphType = CallGraph<ProgramPoint, FunctionContext>;
	const CallGraphType& callGraph;

	const Context* ctx;
	ProgramPointSet& ppSet;

	void addDef(const CFGNode& node, const Value* val)
	{
		assert(val != nullptr);

		if (isa<GlobalValue>(val))
			return;
		auto predNode = node.getCFG().getCFGNodeForValue(val);
		assert(predNode != nullptr);
		ppSet.insert(ProgramPoint(ctx, predNode));
	}
public:
	ValueTracker(const SemiSparseProgram& s, const CallGraphType& cg, const Context* c, ProgramPointSet& p): ssProg(s), callGraph(cg), ctx(c), ppSet(p) {}

	void visitEntryNode(const EntryCFGNode& entryNode)
	{
		auto const& func = entryNode.getFunction();
		auto callers = callGraph.getCallers(FunctionContext(ctx, &func));
		for (auto const& caller: callers)
			ppSet.insert(caller);
	}

	void visitCopyNode(const CopyCFGNode& copyNode)
	{
		for (auto src: copyNode)
			addDef(copyNode, src);
	}
	void visitOffsetNode(const OffsetCFGNode& offsetNode)
	{
		addDef(offsetNode, offsetNode.getSrc());
	}

	void visitCallNode(const CallCFGNode& callNode)
	{
		auto callees = callGraph.getCallees(ProgramPoint(ctx, &callNode));
		for (auto const& callee: callees)
		{
			auto cfg = ssProg.getCFGForFunction(*callee.getFunction());
			assert(cfg != nullptr);
			if (!cfg->doesNotReturn())
			{
				auto retNode = cfg->getExitNode();
				ppSet.insert(ProgramPoint(callee.getContext(), retNode));
			}
		}
	}

	void visitReturnNode(const ReturnCFGNode& retNode)
	{
		for (auto defNode: retNode.defs())
			ppSet.insert(ProgramPoint(ctx, defNode));
	}

	// These nodes are no-ops
	void visitAllocNode(const AllocCFGNode&) {}
	void visitLoadNode(const LoadCFGNode&) {}
	void visitStoreNode(const StoreCFGNode&) {}
};

}

ProgramPointSet ValueDependenceTracker::getValueDependencies(const ProgramPoint& pp) const
{
	ProgramPointSet ppSet;

	auto ctx = pp.getContext();
	auto node = pp.getCFGNode();

	ValueTracker(ssProg, callGraph, ctx, ppSet).visit(*node);

	return ppSet;
}

}
