#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/DataFlow/TaintTransferFunction.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"

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
	if (isa<Constant>(val))
		return TaintLattice::Untainted;
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
	assert(!ptsSet.isEmpty());
	auto loc = *ptsSet.begin();
	
	if (ptsSet.getSize() == 1 && !loc->isSummaryLocation())
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
	assert(!ptsSet.isEmpty());

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

bool TaintTransferFunction::memcpyTaint(PtsSet dstSet, PtsSet srcSet)
{
	bool changed = false;
	auto& memManager = globalState.getPointerAnalysis().getMemoryManager();

	for (auto srcLoc: srcSet)
	{
		auto srcLocs = memManager.getAllOffsetLocations(srcLoc);
		auto startingOffset = srcLoc->getOffset();
		for (auto oLoc: srcLocs)
		{
			auto oVal = store->lookup(oLoc);
			if (oVal == TaintLattice::Unknown)
				continue;

			auto offset = oLoc->getOffset() - startingOffset;
			for (auto updateLoc: dstSet)
			{
				auto tgtLoc = memManager.offsetMemory(updateLoc, offset);
				if (memManager.isSpecialMemoryLocation(tgtLoc))
					break;
				changed |= store->weakUpdate(tgtLoc, oVal);
			}
		}
	}

	return changed;
}

bool TaintTransferFunction::copyTaint(const Value* dst, const Value* src)
{
	bool changed = false;
	auto& env = globalState.getEnv();

	auto srcVal = env.lookup(ProgramLocation(ctx, src));
	if (srcVal != TaintLattice::Unknown)
		changed = env.strongUpdate(ProgramLocation(ctx, dst), srcVal);

	return changed;
}

EvalStatus TaintTransferFunction::evalMemcpy(const Instruction* inst)
{
	ImmutableCallSite cs(inst);

	auto const& ptrAnalysis = globalState.getPointerAnalysis();

	auto dstSet = ptrAnalysis.getPtsSet(ctx, cs.getArgument(0));
	auto srcSet = ptrAnalysis.getPtsSet(ctx, cs.getArgument(1));
	assert(!dstSet.isEmpty() && !srcSet.isEmpty());

	auto storeChanged = memcpyTaint(dstSet, srcSet);
	auto envChanged = copyTaint(inst, cs.getArgument(0));
	return EvalStatus::getValidStatus(envChanged, storeChanged);
}

EvalStatus TaintTransferFunction::evalMalloc(const Instruction* inst)
{
	auto dstSet = globalState.getPointerAnalysis().getPtsSet(ctx, inst);
	assert(dstSet.getSize() == 1);
	auto envChanged = globalState.getEnv().strongUpdate(ProgramLocation(ctx, inst), TaintLattice::Untainted);
	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TaintTransferFunction::taintValueByTClass(const Value* val, TClass taintClass, TaintLattice taintVal)
{
	auto envChanged = false, storeChanged = false;
	switch (taintClass)
	{
		case TClass::ValueOnly:
		{
			envChanged |= globalState.getEnv().strongUpdate(ProgramLocation(ctx, val), taintVal);
			break;
		}
		case TClass::DirectMemory:
		{
			auto const& ptrAnalysis = globalState.getPointerAnalysis();
			auto const& memManager = ptrAnalysis.getMemoryManager();
			auto pSet = ptrAnalysis.getPtsSet(ctx, val);
			for (auto loc: pSet)
			{
				if (memManager.isSpecialMemoryLocation(loc))
					continue;
				if (loc->isSummaryLocation())
					storeChanged |= store->weakUpdate(loc, taintVal);
				else
					storeChanged |= store->strongUpdate(loc, taintVal);
			}
		}
	}
	return EvalStatus::getValidStatus(envChanged, storeChanged);
}

EvalStatus TaintTransferFunction::taintCallByEntry(const Instruction* inst, const TEntry& entry)
{
	ImmutableCallSite cs(inst);
	switch (entry.pos)
	{
		case TPosition::Ret:
			return taintValueByTClass(inst, entry.what, entry.val);
		case TPosition::Arg0:
			return taintValueByTClass(cs.getArgument(0), entry.what, entry.val);
		case TPosition::Arg1:
			return taintValueByTClass(cs.getArgument(1), entry.what, entry.val);
		case TPosition::Arg2:
			return taintValueByTClass(cs.getArgument(2), entry.what, entry.val);
		case TPosition::Arg3:
			return taintValueByTClass(cs.getArgument(3), entry.what, entry.val);
		case TPosition::Arg4:
			return taintValueByTClass(cs.getArgument(4), entry.what, entry.val);
		case TPosition::AfterArg0:
		{
			auto res = EvalStatus::getValidStatus(false, false);
			for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
				res = res || taintValueByTClass(cs.getArgument(i), entry.what, entry.val);

			return res;
		}
		case TPosition::AfterArg1:
		{
			auto res = EvalStatus::getValidStatus(false, false);
			for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
				res = res || taintValueByTClass(cs.getArgument(i), entry.what, entry.val);

			return res;
		}
		case TPosition::AllArgs:
		{
			auto res = EvalStatus::getValidStatus(false, false);
			for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
				res = res || taintValueByTClass(cs.getArgument(i), entry.what, entry.val);

			return res;
		}
	}
}

EvalStatus TaintTransferFunction::evalCallBySummary(const Instruction* inst, const TSummary& summary)
{
	auto res = EvalStatus::getValidStatus(false, false);
	for (auto const& entry: summary)
	{
		// We only care about taint source here
		if (entry.end == TEnd::Sink)
			continue;

		res = res || taintCallByEntry(inst, entry);
	}
	return res;
}

EvalStatus TaintTransferFunction::evalExternalCall(const Instruction* inst, const Function* callee)
{
	assert(store != nullptr);

	auto funName = callee->getName();
	auto ptrEffect = globalState.getExternalPointerEffectTable().lookup(funName);

	if (ptrEffect == PointerEffect::MemcpyArg1ToArg0 || funName == "strcpy" || funName == "strncpy")
		return evalMemcpy(inst);
	else if (ptrEffect == PointerEffect::Malloc)
		return evalMalloc(inst);
	else if (auto summary = globalState.getSourceSinkLookupTable().getSummary(funName))
		return evalCallBySummary(inst, *summary);
	else if (!callee->getReturnType()->isVoidTy())
	{
		auto envChanged = globalState.getEnv().weakUpdate(ProgramLocation(ctx, inst), TaintLattice::Untainted);
		return EvalStatus::getValidStatus(envChanged, false);
	}
	else
		return EvalStatus::getValidStatus(false, false);
}

}
}
