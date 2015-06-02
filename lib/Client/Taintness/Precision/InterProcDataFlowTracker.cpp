#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/InterProcDataFlowTracker.h"
#include "Client/Taintness/Precision/PrecisionLossGlobalState.h"
#include "PointerAnalysis/DataFlow/DefUseProgramLocation.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

static const Value* getArgAtPos(const Instruction* inst, size_t idx)
{
	ImmutableCallSite cs(inst);

	return cs.getArgument(idx);
}

InterProcDataFlowTracker::InterProcDataFlowTracker(const TaintGlobalState& g, PrecisionLossGlobalState& p, GlobalWorkList& w): globalState(g), precGlobalState(p), workList(w)
{
}

TaintLattice InterProcDataFlowTracker::getTaintValue(const Context* ctx, const Value* val)
{
	auto ret = globalState.getEnv().lookup(ProgramLocation(ctx, val));
	assert(ret != TaintLattice::Unknown);
	return ret;
}

TaintLattice InterProcDataFlowTracker::getTaintValue(const DefUseProgramLocation& pLoc, const MemoryLocation* loc)
{
	auto store = globalState.getMemo().lookup(pLoc);
	assert(store != nullptr);

	auto ret = store->lookup(loc);
	return ret;
}

InterProcDataFlowTracker::IndexVector InterProcDataFlowTracker::getDemandingIndices(const TaintVector& taints)
{
	assert(!taints.empty());
	IndexVector ret;

	auto mergeVal = TaintLattice::Unknown;
	for (auto t: taints)
		mergeVal = Lattice<TaintLattice>::merge(mergeVal, t);

	if (mergeVal == TaintLattice::Either)
	{
		for (size_t i = 0, e = taints.size(); i < e; ++i)
			if (taints[i] == TaintLattice::Tainted)
				ret.push_back(i);
	}

	return ret;
}

InterProcDataFlowTracker::IndexVector InterProcDataFlowTracker::getImpreciseIndices(const TaintVector& taints)
{
	assert(!taints.empty());
	IndexVector ret;

	for (size_t i = 0, e = taints.size(); i < e; ++i)
		if (taints[i] == TaintLattice::Either)
			ret.push_back(i);

	return ret;
}

const DefUseFunction& InterProcDataFlowTracker::getDefUseFunction(const Function* f)
{
	return globalState.getProgram().getDefUseFunction(f);
}

const DefUseFunction& InterProcDataFlowTracker::getDefUseFunction(const Instruction* inst)
{
	return getDefUseFunction(inst->getParent()->getParent());
}

void InterProcDataFlowTracker::enqueueWorkList(const Context* ctx, const DefUseFunction* duFunc, const DefUseInstruction* duInst)
{
	workList.enqueue(ctx, duFunc, duInst);
}

void InterProcDataFlowTracker::addImprecisionSource(const DefUseProgramLocation& pLoc)
{
	precGlobalState.addImprecisionSource(pLoc);
}

ReturnDataFlowTracker::ReturnDataFlowTracker(const TaintGlobalState& g, PrecisionLossGlobalState& p, GlobalWorkList& w): InterProcDataFlowTracker(g, p, w)
{
}

ReturnDataFlowTracker::ReturnVector ReturnDataFlowTracker::getReturnInsts(const CallTargetVector& callees)
{
	return vectorTransform(
		callees,
		[this] (auto const& callTgt)
		{
			auto& duFunc = getDefUseFunction(callTgt.second);
			auto exitInst = duFunc.getExitInst();
			return DefUseProgramLocation(callTgt.first, exitInst);
		}
	);
}

ReturnDataFlowTracker::TaintVector ReturnDataFlowTracker::getReturnTaintValues(const ReturnVector& returnSources)
{
	return vectorTransform(
		returnSources,
		[this] (auto const& returnSource)
		{
			auto retDefUseInst = returnSource.getDefUseInstruction();
			auto retInst = retDefUseInst->getInstruction();
			auto retValue = cast<ReturnInst>(retInst)->getReturnValue();
			assert(retValue != nullptr);

			return getTaintValue(returnSource.getContext(), retValue);
		}
	);
}

ReturnDataFlowTracker::TaintVector ReturnDataFlowTracker::getReturnTaintValues(const MemoryLocation* loc, const ReturnVector& returnSources)
{
	return vectorTransform(
		returnSources,
		[this, loc] (auto const& returnSource)
		{
			return getTaintValue(returnSource, loc);
		}
	);
}

bool ReturnDataFlowTracker::isCallSiteNeedMorePrecision(const TaintVector& retTaints)
{
	return !getDemandingIndices(retTaints).empty();
}

void ReturnDataFlowTracker::propagateReturns(const ReturnVector& returnSources, const TaintVector& returnTaints)
{
	assert(returnSources.size() == returnTaints.size());

	auto indices = getImpreciseIndices(returnTaints);
	for (auto idx: indices)
	{
		auto const& returnSource = returnSources[idx];
		auto duInst = returnSource.getDefUseInstruction();
		auto& duFunc = getDefUseFunction(duInst->getFunction());
		enqueueWorkList(returnSource.getContext(), &duFunc, duInst);
	}
}

void ReturnDataFlowTracker::processReturns(const DefUseProgramLocation& pLoc, const ReturnVector& returnSources, const TaintVector& returnTaints)
{
	if (isCallSiteNeedMorePrecision(returnTaints))
		addImprecisionSource(pLoc);

	propagateReturns(returnSources, returnTaints);
}

void ReturnDataFlowTracker::trackValue(const DefUseProgramLocation& pLoc, const ReturnVector& returnSources)
{
	auto duInst = pLoc.getDefUseInstruction();
	auto inst = duInst->getInstruction();

	if (inst->getType()->isVoidTy())
		return;

	auto retTaints = getReturnTaintValues(returnSources);

	processReturns(pLoc, returnSources, retTaints);
}

void ReturnDataFlowTracker::trackMemory(const DefUseProgramLocation& pLoc, const MemoryLocation* loc, const ReturnVector& returnSources)
{
	auto retTaints = getReturnTaintValues(loc, returnSources);
	processReturns(pLoc, returnSources, retTaints);
}

void ReturnDataFlowTracker::trackMemories(const DefUseProgramLocation& pLoc, const ReturnVector& returnSources)
{
	MemorySet memSet;
	for (auto const& mapping: pLoc.getDefUseInstruction()->mem_succs())
		memSet.insert(mapping.first);

	for (auto loc: memSet)
		trackMemory(pLoc, loc, returnSources);
}

void ReturnDataFlowTracker::trackReturn(const DefUseProgramLocation& pLoc, const CallTargetVector& callees)
{
	auto returnSources = getReturnInsts(callees);
	trackValue(pLoc, returnSources);
	trackMemories(pLoc, returnSources);
}


CallDataFlowTracker::CallDataFlowTracker(const TaintGlobalState& g, PrecisionLossGlobalState& p, GlobalWorkList& w): InterProcDataFlowTracker(g, p, w)
{
}

CallDataFlowTracker::DefUseCallerVector CallDataFlowTracker::getDefUseCallerVector(const CallerVector& callers)
{
	return vectorTransform(
		callers, 
		[this] (auto const& callTgt)
		{
			auto inst = callTgt.second;
			auto& duFunc = getDefUseFunction(inst);
			auto duInst = duFunc.getDefUseInstruction(inst);
			return DefUseProgramLocation(callTgt.first, duInst);
		}
	);
}

CallDataFlowTracker::TaintVector CallDataFlowTracker::getArgTaintValues(const DefUseCallerVector& callers, size_t idx)
{
	TaintVector ret;
	ret.reserve(callers.size());

	for (auto callTgt: callers)
	{
		auto arg = getArgAtPos(callTgt.getDefUseInstruction()->getInstruction(), idx);
		auto taint = getTaintValue(callTgt.getContext(), arg);
		ret.push_back(taint);
	}

	return ret;
}

void CallDataFlowTracker::processCaller(const DefUseCallerVector& callers, const TaintVector& callTaints)
{
	auto demandingIndices = getDemandingIndices(callTaints);
	for (auto idx: demandingIndices)
		addImprecisionSource(callers[idx]);

	auto impreciseIndices = getImpreciseIndices(callTaints);
	for (auto idx: impreciseIndices)
	{
		auto& callTgt = callers[idx];
		auto duInst = callTgt.getDefUseInstruction();
		auto& duFunc = getDefUseFunction(duInst->getInstruction());
		enqueueWorkList(callTgt.getContext(), &duFunc, duInst);
	}
}

void CallDataFlowTracker::trackValue(const DefUseFunction* duFunc, const DefUseCallerVector& callers)
{
	auto callee = &duFunc->getFunction();
	auto numArgs = callee->arg_size();
	if (numArgs == 0)
		return;

	for (auto i = 0u; i < numArgs; ++i)
	{
		auto callerArgs = getArgTaintValues(callers, i);

		processCaller(callers, callerArgs);
	}
}

CallDataFlowTracker::TaintVector CallDataFlowTracker::getMemoryTaintValues(const DefUseCallerVector& callers, const MemoryLocation* loc)
{
	auto retVec = std::vector<TaintLattice>();
	retVec.reserve(callers.size());

	for (auto const& callTgt: callers)
	{
		auto tVal = getTaintValue(callTgt, loc);
		if (tVal != TaintLattice::Unknown)
			retVec.push_back(tVal);
	}
	return retVec;
}

void CallDataFlowTracker::trackMemory(const MemoryLocation* loc, const DefUseCallerVector& callers)
{
	auto callTaints = getMemoryTaintValues(callers, loc);
	processCaller(callers, callTaints);
}

void CallDataFlowTracker::trackMemories(const DefUseProgramLocation& pLoc, const DefUseCallerVector& callers)
{
	MemorySet memSet;

	for (auto const& mapping: pLoc.getDefUseInstruction()->mem_succs())
		memSet.insert(mapping.first);

	for (auto loc: memSet)
		trackMemory(loc, callers);	
}

void CallDataFlowTracker::trackCall(const DefUseProgramLocation& pLoc, const DefUseFunction* duFunc, const CallerVector& callers)
{
	auto duCallers = getDefUseCallerVector(callers);

	trackValue(duFunc, duCallers);
	trackMemories(pLoc, duCallers);
}

}
}