#include "Client/Taintness/Precision/TaintPrecisionMonitor.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TaintPrecisionMonitor::TaintPrecisionMonitor(const TaintEnv& e): env(e)
{
}

void TaintPrecisionMonitor::trackPrecisionLoss(const ProgramLocation& pLoc, const Context* newCtx, const Function* callee, const TaintLatticeVector& taintVals)
{
	assert(taintVals.size() == callee->arg_size());
	auto& posSet = record.getRecordAt(pLoc);

	auto paramItr = callee->arg_begin();
	for (auto i = 0ul, e = taintVals.size(); i < e; ++i)
	{
		auto currVal = env.lookup(ProgramLocation(newCtx, paramItr));
		auto argVal = taintVals[i];
		
		if (Lattice<TaintLattice>::willMergeSmearTaintness(currVal, argVal))
			posSet.setArgumentLossPosition(i);

		++paramItr;
	}
}

}
}