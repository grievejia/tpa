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

bool TaintTransferFunction::updateDirectMemoryTaint(const Value* val, TaintLattice taintVal)
{
	auto ret = false;
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto const& memManager = ptrAnalysis.getMemoryManager();
	auto pSet = ptrAnalysis.getPtsSet(ctx, val);
	for (auto loc: pSet)
	{
		if (memManager.isSpecialMemoryLocation(loc))
			continue;
		// TODO: perform storng update sometimes
		ret |= store->weakUpdate(loc, taintVal);
	}
	return ret;
}

bool TaintTransferFunction::updateReachableMemoryTaint(const Value* val, TaintLattice taintVal)
{
	auto ret = false;
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto const& memManager = ptrAnalysis.getMemoryManager();
	auto pSet = ptrAnalysis.getPtsSet(ctx, val);
	for (auto loc: pSet)
	{
		if (memManager.isSpecialMemoryLocation(loc))
			continue;

		auto dstLocs = memManager.getAllOffsetLocations(loc);
		// TODO: perform storng update sometimes
		for (auto dstLoc: dstLocs)
		{
			if (memManager.isSpecialMemoryLocation(dstLoc))
				break;
			ret |= store->weakUpdate(dstLoc, taintVal);
		}
	}
	return ret;
}

EvalStatus TaintTransferFunction::updateTaintValueByTClass(const Value* val, TClass taintClass, TaintLattice taintVal)
{
	auto envChanged = false, storeChanged = false;
	switch (taintClass)
	{
		case TClass::ValueOnly:
		{
			envChanged = globalState.getEnv().strongUpdate(ProgramLocation(ctx, val), taintVal);
			break;
		}
		case TClass::DirectMemory:
		{
			storeChanged = updateDirectMemoryTaint(val, taintVal);
			break;
		}
		case TClass::ReachableMemory:
		{
			storeChanged = updateReachableMemoryTaint(val, taintVal);
			break;
		}
	}
	return EvalStatus::getValidStatus(envChanged, storeChanged);
}

EvalStatus TaintTransferFunction::updateTaintCallByTPosition(const Instruction* inst, TPosition taintPos, TClass taintClass, TaintLattice taintVal)
{
	ImmutableCallSite cs(inst);
	assert(cs);

	if (taintPos.isReturnPosition())
		return updateTaintValueByTClass(inst, taintClass, taintVal);
	else
	{
		auto const& argPos = taintPos.getAsArgPosition();
		if (!argPos.isAfterArgPosition())
			return updateTaintValueByTClass(cs.getArgument(argPos.getArgIndex()), taintClass, taintVal);
		else
		{
			auto res = EvalStatus::getValidStatus(false, false);
			for (size_t i = argPos.getArgIndex(), e = cs.arg_size(); i < e; ++i)
				res = res || updateTaintValueByTClass(cs.getArgument(i), taintClass, taintVal);

			return res;
		}	
	}
}

EvalStatus TaintTransferFunction::evalTaintSource(const Instruction* inst, const SourceTaintEntry& entry)
{
	auto tPos = entry.getTaintPosition();
	auto tClass = (tPos.isReturnPosition()) ? TClass::ValueOnly : TClass::DirectMemory;
	return updateTaintCallByTPosition(inst, tPos, tClass, TaintLattice::Tainted);
}

TaintLattice TaintTransferFunction::getTaintValueByTClass(const Value* val, TClass taintClass)
{
	switch (taintClass)
	{
		case TClass::ValueOnly:
			return globalState.getEnv().lookup(ProgramLocation(ctx, val));
		case TClass::DirectMemory:
		{
			assert(store != nullptr);

			auto const& ptrAnalysis = globalState.getPointerAnalysis();

			auto pSet = ptrAnalysis.getPtsSet(ctx, val);
			assert(!pSet.isEmpty());

			TaintLattice retVal = loadTaintFromPtsSet(pSet);
			
			return retVal;
		}
		case TClass::ReachableMemory:
			llvm_unreachable("ReachableMemory shouldn't be handled by getTaintValueByTClass()");
	}
}

EvalStatus TaintTransferFunction::evalMemcpy(const Value* dstVal, const Value* srcVal)
{
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto const& memManager = ptrAnalysis.getMemoryManager();

	auto dstSet = ptrAnalysis.getPtsSet(ctx, dstVal);
	auto srcSet = ptrAnalysis.getPtsSet(ctx, srcVal);
	assert(!dstSet.isEmpty() && !srcSet.isEmpty());

	bool changed = false;
	for (auto srcLoc: srcSet)
	{
		if (memManager.isSpecialMemoryLocation(srcLoc))
			break;

		auto srcLocs = memManager.getAllOffsetLocations(srcLoc);
		auto startingOffset = srcLoc->getOffset();
		for (auto oLoc: srcLocs)
		{
			auto oVal = TaintLattice::Unknown;
			if (oLoc == memManager.getUniversalLocation())
				oVal = TaintLattice::Either;
			else if (oLoc != memManager.getNullLocation())
				oVal = store->lookup(oLoc);
			
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

	return EvalStatus::getValidStatus(false, changed);
}

EvalStatus TaintTransferFunction::evalTaintPipe(const Instruction* inst, const PipeTaintEntry& entry)
{
	ImmutableCallSite cs(inst);

	auto srcPos = entry.getSrcPosition();
	assert(!srcPos.isReturnPosition());
	assert(!srcPos.getAsArgPosition().isAfterArgPosition());
	auto dstPos = entry.getDstPosition();

	auto srcClass = entry.getSrcClass();
	auto dstClass = entry.getDstClass();

	if (srcClass == TClass::ReachableMemory && dstClass == TClass::ReachableMemory)
	{
		// This is the case of memcpy
		assert(!dstPos.isReturnPosition());
		assert(!dstPos.getAsArgPosition().isAfterArgPosition());

		return evalMemcpy(cs.getArgument(dstPos.getAsArgPosition().getArgIndex()), cs.getArgument(srcPos.getAsArgPosition().getArgIndex()));
	}
	else
	{
		auto srcTaint = getTaintValueByTClass(cs.getArgument(srcPos.getAsArgPosition().getArgIndex()), entry.getSrcClass());
		if (srcTaint == TaintLattice::Unknown)
			return EvalStatus::getInvalidStatus();

		return updateTaintCallByTPosition(inst, dstPos, dstClass, srcTaint);
	}

	
}

EvalStatus TaintTransferFunction::evalCallBySummary(const DefUseInstruction* duInst, const Function* callee, const TaintSummary& summary)
{
	auto res = EvalStatus::getValidStatus(false, false);
	bool isSink = false;
	for (auto const& entry: summary)
	{
		switch (entry.getEntryEnd())
		{
			case TEnd::Source:
				res = res || evalTaintSource(duInst->getInstruction(), entry.getAsSourceEntry());
				break;
			case TEnd::Pipe:
				res = res || evalTaintPipe(duInst->getInstruction(), entry.getAsPipeEntry());
				break;
			case TEnd::Sink:
				// We only care about taint source, but we need to record all taint sinks for future examination
				isSink = true;
				break;
		}
	}

	if (isSink)
		globalState.insertSink(DefUseProgramLocation(ctx, duInst), callee);
	
	return res;
}

EvalStatus TaintTransferFunction::evalExternalCall(const DefUseInstruction* duInst, const Function* callee)
{
	assert(store != nullptr);

	auto funName = callee->getName();
	if (auto summary = globalState.getSourceSinkLookupTable().lookup(funName))
		return evalCallBySummary(duInst, callee, *summary);
	else
	{
		errs() << "Missing annotation for external function " << funName << "\n";
		llvm_unreachable("Library call not handled");
	}
}

}
}