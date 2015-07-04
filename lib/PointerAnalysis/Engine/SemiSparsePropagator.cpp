#include "PointerAnalysis/Engine/EvalResult.h"
#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/SemiSparsePropagator.h"
#include "PointerAnalysis/Engine/WorkList.h"
#include "PointerAnalysis/Program/CFG/CFG.h"
#include "PointerAnalysis/Support/Memo.h"

namespace tpa
{

namespace
{

bool isTopLevelNode(const CFGNode* node)
{
	return node->isAllocNode() || node->isCopyNode() || node->isOffsetNode();
}

}

void SemiSparsePropagator::enqueueIfMemoChange(const ProgramPoint& pp, const Store& store)
{
	if (memo.update(pp, store))
		workList.enqueue(pp);
}

void SemiSparsePropagator::propagateEntryNode(const ProgramPoint& pp, const Store& store)
{
	// To prevent premature fixpoint, we need to enqueue the return node here
	auto const& cfg = pp.getCFGNode()->getCFG();
	if (!cfg.doesNotReturn())
	{
		auto exitNode = cfg.getExitNode();
		workList.enqueue(ProgramPoint(pp.getContext(), exitNode));
	}

	enqueueIfMemoChange(pp, store);
}

void SemiSparsePropagator::propagate(const EvalResult& evalResult)
{
	auto const& store = evalResult.getStore();
	for (auto const& evalSucc: evalResult)
	{
		if (evalSucc.isTopLevel())
		{
			workList.enqueue(evalSucc.getProgramPoint());
			continue;
		}

		assert(!isTopLevelNode(evalSucc.getProgramPoint().getCFGNode()));

		// We need to do something special for entry and return node
		auto const& pp = evalSucc.getProgramPoint();
		if (pp.getCFGNode()->isEntryNode())
			propagateEntryNode(pp, store);
		else
			enqueueIfMemoChange(pp, store);
	}
}

}
