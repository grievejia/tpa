#include "TaintAnalysis/Precision/ReturnTracker.h"
#include "TaintAnalysis/Precision/TrackerGlobalState.h"
#include "TaintAnalysis/Precision/TrackerWorkList.h"
#include "TaintAnalysis/Precision/VectorTransform.h"
#include "TaintAnalysis/Program/DefUseModule.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintMemo.h"

#include <llvm/IR/Instructions.h>

using namespace llvm;

namespace taint
{

ReturnTracker::ReturnVector ReturnTracker::getReturnVector(const CalleeVector& callees)
{
	return vectorTransform(
		callees,
		[this] (auto const& fc)
		{
			auto& duFunc = trackerState.getDefUseModule().getDefUseFunction(fc.getFunction());
			auto exitInst = duFunc.getExitInst();
			return ProgramPoint(fc.getContext(), exitInst);
		}
	);
}

TaintVector ReturnTracker::getReturnTaintValues(const ReturnVector& retVec)
{
	return vectorTransform(
		retVec,
		[this] (auto const& returnSource)
		{
			auto retInst = returnSource.getDefUseInstruction()->getInstruction();
			auto retValue = cast<ReturnInst>(retInst)->getReturnValue();
			assert(retValue != nullptr);

			auto tVal = trackerState.getEnv().lookup(TaintValue(returnSource.getContext(), retValue));
			//assert(tVal != TaintLattice::Unknown);
			return tVal;
		}
	);
}

TaintVector ReturnTracker::getReturnTaintValues(const tpa::MemoryObject* obj, const ReturnVector& retVec)
{
	return vectorTransform(
		retVec,
		[this, obj] (auto const& returnSource)
		{
			auto store = trackerState.getMemo().lookup(returnSource);
			assert(store != nullptr);
			auto tVal = store->lookup(obj);
			return tVal;
		}
	);
}

void ReturnTracker::propagateReturns(const ReturnVector& retVec, const TaintVector& returnTaints)
{
	assert(retVec.size() == returnTaints.size());

	auto indices = getImpreciseIndices(returnTaints);
	for (auto idx: indices)
		workList.enqueue(retVec[idx]);
}

void ReturnTracker::processReturns(const ProgramPoint& pp, const ReturnVector& retVec, const TaintVector& returnTaints)
{
	if (!getDemandingIndices(returnTaints).empty())
		trackerState.addImprecisionSource(pp);

	propagateReturns(retVec, returnTaints);
}

void ReturnTracker::trackValue(const ProgramPoint& pp, const ReturnVector& retVec)
{
	auto inst = pp.getDefUseInstruction()->getInstruction();

	if (inst->getType()->isVoidTy())
		return;

	auto retTaints = getReturnTaintValues(retVec);

	processReturns(pp, retVec, retTaints);
}

void ReturnTracker::trackMemory(const ProgramPoint& pp, const ReturnVector& retVec)
{
	for (auto const& mapping: pp.getDefUseInstruction()->mem_succs())
	{
		auto retTaints = getReturnTaintValues(mapping.first, retVec);
		processReturns(pp, retVec, retTaints);
	}
}

void ReturnTracker::trackReturn(const ProgramPoint& pp, const CalleeVector& callees)
{
	auto retVec = getReturnVector(callees);
	trackValue(pp, retVec);
	trackMemory(pp, retVec);
}

}