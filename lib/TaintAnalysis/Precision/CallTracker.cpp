#include "TaintAnalysis/Precision/CallTracker.h"
#include "TaintAnalysis/Precision/LocalTracker.h"
#include "TaintAnalysis/Precision/TrackerGlobalState.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintMemo.h"
#include "Util/DataStructure/VectorSet.h"
#include "Util/IO/TaintAnalysis/Printer.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace util::io;

namespace taint
{

static const Value* getArgAtPos(const Instruction* inst, size_t idx)
{
	ImmutableCallSite cs(inst);
	assert(cs);

	return cs.getArgument(idx);
}

TaintVector CallTracker::getArgTaintValues(const CallerVector& callers, size_t idx)
{
	TaintVector ret;
	ret.reserve(callers.size());

	for (auto const& callsite: callers)
	{
		auto arg = getArgAtPos(callsite.getDefUseInstruction()->getInstruction(), idx);
		auto taint = trackerState.getEnv().lookup(TaintValue(callsite.getContext(), arg));
		if (taint == TaintLattice::Unknown)
			errs() << *callsite.getDefUseInstruction()->getInstruction() << "\n";
		assert(taint != TaintLattice::Unknown);
		ret.push_back(taint);
	}

	return ret;
}

void CallTracker::trackValue(const ProgramPoint& pp, const CallerVector& callers)
{
	auto callee = pp.getDefUseInstruction()->getFunction();
	auto numArgs = callee->arg_size();
	if (numArgs == 0)
		return;

	std::unordered_map<size_t, util::VectorSet<const llvm::Value*>> trackedValues;
	for (auto i = 0u; i < numArgs; ++i)
	{
		auto callTaints = getArgTaintValues(callers, i);

		auto demandingIndices = getDemandingIndices(callTaints);
		for (auto idx: demandingIndices)
			trackerState.addImprecisionSource(callers[idx]);

		auto impreciseIndices = getImpreciseIndices(callTaints);
		for (auto idx: impreciseIndices)
		{
			auto arg = getArgAtPos(callers[idx].getDefUseInstruction()->getInstruction(), i);
			trackedValues[idx].insert(arg);
		}
	}

	LocalTracker localTracker(workList);
	for (auto const& mapping: trackedValues)
		localTracker.trackValue(callers[mapping.first], mapping.second);
}

TaintVector CallTracker::getMemoryTaintValues(const CallerVector& callers, const tpa::MemoryObject* obj)
{
	auto retVec = std::vector<TaintLattice>();
	retVec.reserve(callers.size());

	for (auto const& callsite: callers)
	{
		auto store = trackerState.getMemo().lookup(callsite);
		assert(store != nullptr);

		auto tVal = store->lookup(obj);
		retVec.push_back(tVal);
	}
	return retVec;
}

void CallTracker::trackMemory(const ProgramPoint& pp, const CallerVector& callers)
{
	std::unordered_map<size_t, util::VectorSet<const tpa::MemoryObject*>> trackedObjects;
	
	for (auto const& mapping: pp.getDefUseInstruction()->mem_succs())
	{
		auto obj = mapping.first;
		auto callTaints = getMemoryTaintValues(callers, obj);

		auto demandingIndices = getDemandingIndices(callTaints);
		for (auto idx: demandingIndices)
			trackerState.addImprecisionSource(callers[idx]);

		auto impreciseIndices = getImpreciseIndices(callTaints);
		for (auto idx: impreciseIndices)
			trackedObjects[idx].insert(obj);
	}

	LocalTracker localTracker(workList);
	for (auto const& mapping: trackedObjects)
		localTracker.trackMemory(callers[mapping.first], mapping.second);
}

void CallTracker::trackCall(const ProgramPoint& pp, const std::vector<ProgramPoint>& callers)
{
	if (!callers.empty())
	{
		trackValue(pp, callers);
		trackMemory(pp, callers);
	}
}

}