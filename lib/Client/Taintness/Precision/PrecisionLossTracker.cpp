#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/LocalDataFlowTracker.h"
#include "Client/Taintness/Precision/PrecisionLossTracker.h"
#include "Client/Taintness/Precision/FunctionPrecisionLossTracker.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

PrecisionLossTracker::PrecisionLossTracker(const TaintGlobalState& g): globalState(g)
{
}

void PrecisionLossTracker::addSinkViolation(const DefUseProgramLocation& pLoc, const SinkViolationRecords& records)
{
	auto duInst = pLoc.getDefUseInstruction();
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
			auto pSet = ptrAnalysis.getPtsSet(pLoc.getContext(), argVal);
			assert(!pSet.isEmpty());

			locs.insert(pSet.begin(), pSet.end());
		}
	}

	auto duFunc = &globalState.getProgram().getDefUseFunction(duInst->getFunction());
	auto& localWorkList = globalWorkList.enqueueAndGetLocalWorkList(pLoc.getContext(), duFunc);

	if (!values.empty())
		LocalDataFlowTracker(localWorkList).trackValues(pLoc.getDefUseInstruction(), values);
	if (!locs.empty())
		LocalDataFlowTracker(localWorkList).trackMemory(pLoc.getDefUseInstruction(), locs);
}

DefUseProgramLocationSet PrecisionLossTracker::trackImprecisionSource()
{
	DefUseProgramLocationSet retSet;
	PrecisionLossGlobalState precGlobalState(retSet);

	while (!globalWorkList.isEmpty())
	{
		const Context* ctx;
		const DefUseFunction* duFunc;
		std::tie(ctx, duFunc) = globalWorkList.dequeue();

		FunctionPrecisionLossTracker(ctx, duFunc, globalState, precGlobalState, globalWorkList).track();
	}

	return retSet;
}

}
}