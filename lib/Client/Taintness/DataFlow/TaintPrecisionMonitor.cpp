#include "Client/Taintness/DataFlow/TaintPrecisionMonitor.h"
#include "MemoryModel/Precision/AdaptiveContext.h"

#include <llvm/IR/Function.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

void TaintPrecisionMonitor::trackCallSite(const ProgramLocation& pLoc, const llvm::Function* callee, const Context* ctx, const TaintEnv& env, const TaintLatticeList& argList)
{
	if (trackedCallSite.count(pLoc))
		return;
	auto paramItr = callee->arg_begin();
	for (auto argVal: argList)
	{
		auto optVal = env.lookup(ProgramLocation(ctx, paramItr));
		if (optVal)
		{
			if (Lattice<TaintLattice>::compare(*optVal, argVal) == LatticeCompareResult::Incomparable)
			{
				trackedCallSite.insert(pLoc);
				break;
			}
		}
		++paramItr;
	}
}

void TaintPrecisionMonitor::changeContextSensitivity() const
{
	for (auto const& pLoc: trackedCallSite)
		AdaptiveContext::trackCallSite(pLoc);
}

}
}
