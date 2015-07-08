#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "TaintAnalysis/Engine/TaintGlobalState.h"
#include "TaintAnalysis/Precision/LocalTracker.h"
#include "TaintAnalysis/Precision/PrecisionLossTracker.h"
#include "TaintAnalysis/Precision/TrackerGlobalState.h"
#include "TaintAnalysis/Precision/TrackerTransferFunction.h"
#include "TaintAnalysis/Precision/TrackerWorkList.h"

#include <llvm/IR/CallSite.h>

#include <unordered_set>

namespace taint
{

void PrecisionLossTracker::initializeWorkList(TrackerWorkList& workList, const SinkViolationRecord& record)
{
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	for (auto const& mapping: record)
	{
		// We only care about imprecision
		auto const& pp = mapping.first;
		llvm::ImmutableCallSite cs(pp.getDefUseInstruction()->getInstruction());
		assert(cs);
		auto ctx = pp.getContext();
		std::unordered_set<const llvm::Value*> trackedValues;
		std::unordered_set<const tpa::MemoryObject*> trackedObjects;
		
		for (auto const& violation: mapping.second)
		{
			if (violation.actualVal != TaintLattice::Either)
				continue;

			auto argVal = cs.getArgument(violation.argPos);
			if (violation.what == annotation::TClass::ValueOnly)
			{
				trackedValues.insert(argVal);
			}
			else
			{
				auto pSet = ptrAnalysis.getPtsSet(ctx, argVal);
				assert(!pSet.empty());

				trackedObjects.insert(pSet.begin(), pSet.end());
			}
		}

		LocalTracker tracker(workList);
		tracker.trackValue(pp, trackedValues);
		tracker.trackMemory(pp, trackedObjects);
	}
}

ProgramPointSet PrecisionLossTracker::trackImprecision(const SinkViolationRecord& record)
{
	ProgramPointSet ppSet;

	// Prepare the tracker state
	TrackerGlobalState trackerState(globalState.getDefUseModule(), globalState.getPointerAnalysis(), globalState.getExternalTaintTable(), globalState.getEnv(), globalState.getMemo(), globalState.getCallGraph(), ppSet);

	// Prepare the tracker worklist
	TrackerWorkList workList;
	initializeWorkList(workList, record);

	// The main analysis
	while (!workList.empty())
	{
		auto pp = workList.dequeue();

		TrackerTransferFunction(trackerState, workList).eval(pp);
	}

	return ppSet;
}

}