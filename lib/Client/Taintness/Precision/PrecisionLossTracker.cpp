#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/PrecisionLossTracker.h"
#include "Client/Taintness/Precision/TrackingTransferFunction.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/CallSite.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

void PrecisionLossTracker::processSinkViolationRecords(const DefUseInstruction* duInst, const SinkViolationRecords& records)
{
	ImmutableCallSite cs(duInst->getInstruction());
	auto const& ptrAnalysis = globalState.getPointerAnalysis();

	ValueSet values;
	MemorySet locs;
	for (auto record: records)
	{
		// We only care about imprecision
		if (record.actualVal != TaintLattice::Either)
			continue;

		auto argVal = cs.getArgument(record.argPos);
		if (record.what == TClass::ValueOnly)
		{
			values.insert(argVal);
		}
		else
		{
			auto pSet = ptrAnalysis.getPtsSet(ctx, argVal);
			assert(!pSet.isEmpty());

			locs.insert(pSet.begin(), pSet.end());
		}
	}

	if (!values.empty())
		trackValues(duInst, values);
	if (!locs.empty())
		trackMemory(duInst, locs);
}

PrecisionLossTracker::PrecisionLossTracker(const ContextDefUseFunction& cf, const FunctionSinkViolationRecords& funRecords, const TaintGlobalState& g): globalState(g), ctx(cf.getContext()), duFunc(cf.getDefUseFunction())
{
	for (auto const& funRecord: funRecords)
	{
		auto duInst = funRecord.getDefUseInstruction();
		auto const& records = funRecord.getSinkViolationRecords();
		processSinkViolationRecords(duInst, records);
	}
}

void PrecisionLossTracker::PrecisionLossTracker::trackValues(const DefUseInstruction* duInst, const ValueSet& values)
{
	assert(!duInst->isEntryInstruction());

	// Detect tainted arguments first
	for (auto value: values)
	{
		if (auto arg = dyn_cast<Argument>(value))
			trackedArguments.insert(arg);
	}

	// Enqueue all affected operands
	for (auto predDuInst: duInst->top_preds())
	{
		if (predDuInst->isEntryInstruction())
			continue;
		
		auto predInst = predDuInst->getInstruction();
		if (values.count(predInst))
			workList.enqueue(predDuInst);
	}
}

void PrecisionLossTracker::PrecisionLossTracker::trackMemory(const DefUseInstruction* duInst, const MemorySet& locs)
{
	assert(!duInst->isEntryInstruction());

	// Enqueue all affected operands
	for (auto const& mapping: duInst->mem_preds())
	{
		if (!locs.count(mapping.first))
			continue;
		
		for (auto predDuInst: mapping.second)
		{
			if (predDuInst->isEntryInstruction())
				trackedExternalLocations.insert(mapping.first);
			else
				workList.enqueue(predDuInst);
		}
	}
}

void PrecisionLossTracker::trackSource(const DefUseInstruction* duInst)
{
	ValueSet values;
	MemorySet locs;
	TrackingTransferFunction(globalState, values, locs).eval(duInst);

	if (!values.empty())
		trackValues(duInst, values);
	if (!locs.empty())
		trackMemory(duInst, locs);
}

PrecisionLossTracker::CallSiteSet PrecisionLossTracker::trackPrecisionLossSource()
{
	CallSiteSet callsiteSet;

	while (!workList.isEmpty())
	{
		auto duInst = workList.dequeue();
		assert(!duInst->isEntryInstruction());

		auto visited = visitedDuInstructions.insert(duInst).second;
		if (!visited)
			continue;

		trackSource(duInst);
	}

	return callsiteSet;
}

}
}