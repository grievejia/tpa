#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/DataFlow/TaintTransferFunction.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TaintTransferFunction::TaintTransferFunction(const Context* c, TaintGlobalState& g): ctx(c), globalState(g)
{
}
TaintTransferFunction::TaintTransferFunction(const Context* c, TaintStore& s, TaintGlobalState& g): ctx(c), store(&s), globalState(g)
{
}

TaintLattice TaintTransferFunction::getTaintForValue(const Value* val)
{
	return globalState.getEnv().lookup(ProgramLocation(ctx, val));
}

TaintLattice TaintTransferFunction::getTaintForOperands(const Instruction* inst)
{
	TaintLattice currVal = TaintLattice::Unknown;
	for (auto i = 0u, e = inst->getNumOperands(); i < e; ++i)
	{
		auto op = inst->getOperand(i);
		auto opVal = getTaintForValue(op);
		currVal = Lattice<TaintLattice>::merge(currVal, opVal);
	}
	return currVal;
}

EvalStatus TaintTransferFunction::evalAlloca(const llvm::Instruction* inst)
{
	auto allocInst = cast<AllocaInst>(inst);

	auto envChanged = globalState.getEnv().strongUpdate(ProgramLocation(ctx, allocInst), TaintLattice::Untainted);

	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TaintTransferFunction::evalAllOperands(const Instruction* inst)
{
	auto val = getTaintForOperands(inst);
	if (val != TaintLattice::Unknown)
	{
		auto envChanged = globalState.getEnv().strongUpdate(ProgramLocation(ctx, inst), val);
		return EvalStatus::getValidStatus(envChanged, false);
	}
	else
		return EvalStatus::getInvalidStatus();
}

EvalStatus TaintTransferFunction::evalPhiNode(const Instruction* inst)
{
	// Phi nodes has to make progress without all operands information available
	auto val = getTaintForOperands(inst);
	if (val == TaintLattice::Unknown)
		val = TaintLattice::Untainted;
	auto envChanged = globalState.getEnv().strongUpdate(ProgramLocation(ctx, inst), val);
	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TaintTransferFunction::strongUpdateStore(const MemoryLocation* loc, TaintLattice v)
{
	auto storeChanged = store->strongUpdate(loc, v);
	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TaintTransferFunction::weakUpdateStore(PtsSet pSet, TaintLattice v)
{
	bool storeChanged = false;
	for (auto loc: pSet)
	{
		if (globalState.getPointerAnalysis().getMemoryManager().isSpecialMemoryLocation(loc))
			continue;
		storeChanged |= store->weakUpdate(loc, v);
	}
	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TaintTransferFunction::evalStore(const llvm::Instruction* inst)
{
	assert(store != nullptr);

	auto storeInst = cast<StoreInst>(inst);

	auto valOp = storeInst->getValueOperand();
	auto ptrOp = storeInst->getPointerOperand();

	auto val = getTaintForValue(valOp);
	if (val == TaintLattice::Unknown)
		return EvalStatus::getInvalidStatus();

	auto ptsSet = globalState.getPointerAnalysis().getPtsSet(ctx, ptrOp);
	assert(!ptsSet.empty());
	auto loc = *ptsSet.begin();
	
	if (ptsSet.size() == 1 && !loc->isSummaryLocation())
		return strongUpdateStore(loc, val);
	else
		return weakUpdateStore(ptsSet, val);
}

TaintLattice TaintTransferFunction::loadTaintFromPtsSet(PtsSet pSet)
{
	auto uLoc = globalState.getPointerAnalysis().getMemoryManager().getUniversalLocation();
	auto nLoc = globalState.getPointerAnalysis().getMemoryManager().getNullLocation();

	TaintLattice resVal = TaintLattice::Unknown;
	for (auto loc: pSet)
	{
		if (loc == uLoc)
		{
			resVal = TaintLattice::Either;
			break;
		}
		else if (loc == nLoc)
			continue;

		auto locVal = store->lookup(loc);
		resVal = Lattice<TaintLattice>::merge(resVal, locVal);
	}

	return resVal;
}

EvalStatus TaintTransferFunction::evalLoad(const llvm::Instruction* inst)
{
	assert(store != nullptr);

	auto loadInst = cast<LoadInst>(inst);

	auto ptrOp = loadInst->getPointerOperand();
	auto ptsSet = globalState.getPointerAnalysis().getPtsSet(ctx, ptrOp);
	assert(!ptsSet.empty());

	auto loadVal = loadTaintFromPtsSet(ptsSet);
	if (loadVal == TaintLattice::Unknown)
		return EvalStatus::getInvalidStatus();
	else
	{
		auto envChanged = globalState.getEnv().strongUpdate(ProgramLocation(ctx, loadInst), loadVal);
		return EvalStatus::getValidStatus(envChanged, false);
	}
}

std::vector<TaintLattice> TaintTransferFunction::collectArgumentTaintValue(ImmutableCallSite cs, size_t numParam)
{
	std::vector<TaintLattice> callerVals;
	callerVals.reserve(numParam);

	auto const& env = globalState.getEnv();
	for (auto i = 0ul, e = numParam; i < e; ++i)
	{
		auto arg = cs.getArgument(i);
		if (isa<Constant>(arg))
		{
			callerVals.push_back(TaintLattice::Untainted);
			continue;
		}

		auto argVal = env.lookup(ProgramLocation(ctx, arg));
		if (argVal == TaintLattice::Unknown)
			break;
		callerVals.push_back(argVal);
	}
	return callerVals;
}

bool TaintTransferFunction::updateParamTaintValue(const Context* newCtx, const Function* callee, const std::vector<TaintLattice>& argVals)
{
	auto ret = false;
	auto paramItr = callee->arg_begin();
	for (auto argVal: argVals)
	{
		ret |= globalState.getEnv().weakUpdate(ProgramLocation(newCtx, paramItr), argVal);
		++paramItr;
	}
	return ret;
}

EvalStatus TaintTransferFunction::evalCallArguments(const Instruction* inst, const Context* newCtx, const Function* callee)
{
	ImmutableCallSite cs(inst);
	assert(cs);

	auto numParam = callee->arg_size();
	assert(cs.arg_size() >= numParam);

	auto argSets = collectArgumentTaintValue(cs, numParam);
	if (argSets.size() < numParam)
		return EvalStatus::getInvalidStatus();

	auto envChanged = updateParamTaintValue(newCtx, callee, argSets);
	return EvalStatus::getValidStatus(envChanged, false);
}

}
}
