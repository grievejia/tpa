#include "PointerAnalysis/Engine/EvalResult.h"
#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/SemiSparsePropagator.h"
#include "PointerAnalysis/Engine/WorkList.h"
#include "PointerAnalysis/Program/CFG/CFG.h"
#include "PointerAnalysis/Support/Memo.h"

#include "Util/IO/PointerAnalysis/Printer.h"
#include <llvm/Support/raw_ostream.h>
using namespace llvm;

namespace tpa
{

namespace
{

bool isTopLevelNode(const CFGNode* node)
{
	return node->isAllocNode() || node->isCopyNode() || node->isOffsetNode();
}

}

bool SemiSparsePropagator::enqueueIfMemoChange(const ProgramPoint& pp, const Store& store)
{
	if (memo.update(pp, store))
	{
		workList.enqueue(pp);
		return true;
	}
	else
		return false;
}

void SemiSparsePropagator::propagateTopLevel(const EvalSuccessor& evalSucc)
{
	// Top-level successors: no store merging, just enqueue
	workList.enqueue(evalSucc.getProgramPoint());
	//errs() << "\tENQ(T) " << evalSucc.getProgramPoint() << "\n";
}

void SemiSparsePropagator::propagateMemLevel(const EvalSuccessor& evalSucc)
{
	// Mem-level successors: store merging, enqueue if memo changed
	auto node = evalSucc.getProgramPoint().getCFGNode();
	assert(!isTopLevelNode(node));
	assert(evalSucc.getStore() != nullptr);
	bool enqueued = enqueueIfMemoChange(evalSucc.getProgramPoint(), *evalSucc.getStore());

	//if (enqueued)
	//	errs() << "\tENQ(M) " << evalSucc.getProgramPoint() << "\n";
}

void SemiSparsePropagator::propagate(const EvalResult& evalResult)
{
	for (auto const& evalSucc: evalResult)
	{
		if (evalSucc.isTopLevel())
			propagateTopLevel(evalSucc);
		else
			propagateMemLevel(evalSucc);
	}
}

}
