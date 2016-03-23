#include "TaintAnalysis/Engine/EvalResult.h"
#include "TaintAnalysis/Engine/TaintPropagator.h"
#include "TaintAnalysis/Engine/WorkList.h"
#include "TaintAnalysis/Support/TaintMemo.h"

#include "Util/IO/TaintAnalysis/Printer.h"
#include <llvm/Support/raw_ostream.h>
using namespace util::io;

namespace taint
{

void TaintPropagator::enqueueIfMemoChange(const ProgramPoint& pp, const tpa::MemoryObject* obj, const TaintStore& store)
{
	auto objVal = store.lookup(obj);
	if (objVal == TaintLattice::Unknown)
		return;
	if (memo.insert(pp, obj, objVal))
	{
		//llvm::errs() << "\tenqueue mem " << pp << "\n";
		workList.enqueue(pp);
	}
}

void TaintPropagator::propagate(const EvalResult& evalResult)
{
	auto const& store = evalResult.getStore();
	for (auto succ: evalResult)
	{
		if (succ.isTopLevel())
			workList.enqueue(succ.getProgramPoint());
		else
			enqueueIfMemoChange(succ.getProgramPoint(), succ.getMemoryObject(), store);
	}
}

}