#include "Annotation/Taint/ExternalTaintTable.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "TaintAnalysis/Engine/TaintGlobalState.h"
#include "TaintAnalysis/Engine/TransferFunction.h"
#include "TaintAnalysis/Program/DefUseInstruction.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintValue.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;

namespace taint
{

void TransferFunction::updateDirectMemoryTaint(const TaintValue& tv, TaintLattice taintVal, const ProgramPoint& pp, EvalResult& evalResult)
{
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto pSet = ptrAnalysis.getPtsSet(tv.getContext(), tv.getValue());
	auto& store = evalResult.getStore();
	for (auto obj: pSet)
	{
		if (obj->isSpecialObject())
			continue;
		store.weakUpdate(obj, taintVal);
		addMemLevelSuccessors(pp, obj, evalResult);
	}
}

void TransferFunction::updateReachableMemoryTaint(const TaintValue& tv, TaintLattice taintVal, const ProgramPoint& pp, EvalResult& evalResult)
{
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto const& memManager = ptrAnalysis.getMemoryManager();
	auto pSet = ptrAnalysis.getPtsSet(tv.getContext(), tv.getValue());
	auto& store = evalResult.getStore();
	for (auto obj: pSet)
	{
		if (obj->isSpecialObject())
			continue;

		auto dstObjs = memManager.getReachableMemoryObjects(obj);
		for (auto dstObj: dstObjs)
		{
			if (dstObj->isSpecialObject())
				break;
			store.weakUpdate(dstObj, taintVal);
			addMemLevelSuccessors(pp, dstObj, evalResult);
		}
	}
}

void TransferFunction::updateTaintValueByTClass(const TaintValue& tv, TClass taintClass, TaintLattice taintVal, const ProgramPoint& pp, EvalResult& evalResult)
{
	switch (taintClass)
	{
		case TClass::ValueOnly:
		{
			auto envChanged = globalState.getEnv().strongUpdate(tv, taintVal);
			if (envChanged)
				addTopLevelSuccessors(pp, evalResult);
			break;
		}
		case TClass::DirectMemory:
		{
			updateDirectMemoryTaint(tv, taintVal, pp, evalResult);
			break;
		}
		case TClass::ReachableMemory:
		{
			updateReachableMemoryTaint(tv, taintVal, pp, evalResult);
			break;
		}
	}
}

void TransferFunction::updateTaintCallByTPosition(const ProgramPoint& pp, TPosition taintPos, TClass taintClass, TaintLattice taintVal, EvalResult& evalResult)
{
	ImmutableCallSite cs(pp.getDefUseInstruction()->getInstruction());
	assert(cs);

	if (taintPos.isReturnPosition())
	{
		auto tv = TaintValue(pp.getContext(), cs.getInstruction());
		updateTaintValueByTClass(tv, taintClass, taintVal, pp, evalResult);
	}
	else
	{
		auto const& argPos = taintPos.getAsArgPosition();
		if (!argPos.isAfterArgPosition())
		{
			auto argTVal = TaintValue(pp.getContext(), cs.getArgument(argPos.getArgIndex()));
			updateTaintValueByTClass(argTVal, taintClass, taintVal, pp, evalResult);
		}
		else
		{
			for (size_t i = argPos.getArgIndex(), e = cs.arg_size(); i < e; ++i)
			{
				auto argTVal = TaintValue(pp.getContext(), cs.getArgument(i));
				updateTaintValueByTClass(argTVal, taintClass, taintVal, pp, evalResult);
			}
		}	
	}
}

void TransferFunction::evalTaintSource(const ProgramPoint& pp, const SourceTaintEntry& entry, EvalResult& evalResult)
{
	auto tPos = entry.getTaintPosition();
	auto tClass = entry.getTaintClass();
	if (tPos.isReturnPosition() && tClass != TClass::ValueOnly)
		llvm_unreachable("Tainted return source can only be a value!");

	updateTaintCallByTPosition(pp, tPos, tClass, entry.getTaintValue(), evalResult);
}

TaintLattice TransferFunction::getTaintValueByTClass(const TaintValue& tv, TClass tClass)
{
	switch (tClass)
	{
		case TClass::ValueOnly:
			return globalState.getEnv().lookup(tv);
		case TClass::DirectMemory:
		{
			if (localState == nullptr)
				return TaintLattice::Unknown;
			//assert(localState != nullptr);

			auto const& ptrAnalysis = globalState.getPointerAnalysis();
			auto pSet = ptrAnalysis.getPtsSet(tv.getContext(), tv.getValue());
			assert(!pSet.empty());
			TaintLattice retVal = loadTaintFromPtsSet(pSet, *localState);
			
			return retVal;
		}
		case TClass::ReachableMemory:
			llvm_unreachable("ReachableMemory shouldn't be handled by getTaintValueByTClass()");
	}
}

void TransferFunction::evalMemcpy(const TaintValue& srcVal, const TaintValue& dstVal, const ProgramPoint& pp, EvalResult& evalResult)
{
	if (localState == nullptr)
		return;
	//assert(localState != nullptr);

	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto const& memManager = ptrAnalysis.getMemoryManager();

	auto dstSet = ptrAnalysis.getPtsSet(dstVal.getContext(), dstVal.getValue());
	auto srcSet = ptrAnalysis.getPtsSet(srcVal.getContext(), srcVal.getValue());
	assert(!dstSet.empty() && !srcSet.empty());

	auto& store = evalResult.getStore();
	for (auto srcObj: srcSet)
	{
		if (srcObj->isSpecialObject())
			break;

		auto srcObjs = memManager.getReachableMemoryObjects(srcObj);
		auto startingOffset = srcObj->getOffset();
		for (auto oObj: srcObjs)
		{
			auto oVal = TaintLattice::Unknown;
			if (oObj->isUniversalObject())
				oVal = TaintLattice::Either;
			else if (oObj->isNullObject())
				oVal = localState->lookup(oObj);
			
			if (oVal == TaintLattice::Unknown)
				continue;

			auto offset = oObj->getOffset() - startingOffset;
			for (auto updateObj: dstSet)
			{
				auto tgtObj = memManager.offsetMemory(updateObj, offset);
				if (tgtObj->isSpecialObject())
					break;
				store.weakUpdate(tgtObj, oVal);
				addMemLevelSuccessors(pp, tgtObj, evalResult);
			}
		}
	}
}

void TransferFunction::evalTaintPipe(const ProgramPoint& pp, const annotation::PipeTaintEntry& entry, EvalResult& evalResult)
{
	ImmutableCallSite cs(pp.getDefUseInstruction()->getInstruction());
	assert(cs);

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

		auto srcVal = TaintValue(pp.getContext(), cs.getArgument(srcPos.getAsArgPosition().getArgIndex()));
		auto dstVal = TaintValue(pp.getContext(), cs.getArgument(dstPos.getAsArgPosition().getArgIndex()));
		evalMemcpy(dstVal, srcVal, pp, evalResult);
	}
	else
	{
		auto srcVal = TaintValue(pp.getContext(), cs.getArgument(srcPos.getAsArgPosition().getArgIndex()));
		auto srcTaint = getTaintValueByTClass(srcVal, entry.getSrcClass());
		if (srcTaint == TaintLattice::Unknown)
			return;

		updateTaintCallByTPosition(pp, dstPos, dstClass, srcTaint, evalResult);
	}
}


void TransferFunction::evalCallBySummary(const ProgramPoint& pp, const Function* callee, const TaintSummary& summary, EvalResult& evalResult)
{
	bool isSink = false;
	for (auto const& entry: summary)
	{
		switch (entry.getEntryEnd())
		{
			case TEnd::Source:
				evalTaintSource(pp, entry.getAsSourceEntry(), evalResult);
				break;
			case TEnd::Pipe:
				evalTaintPipe(pp, entry.getAsPipeEntry(), evalResult);
				break;
			case TEnd::Sink:
				// We only care about taint source, but we need to record all taint sinks for future examination
				isSink = true;
				break;
		}
	}

	if (isSink)
		globalState.getSinks().insert(SinkSignature(pp, callee));
}

void TransferFunction::evalExternalCall(const ProgramPoint& pp, const Function* func, EvalResult& evalResult)
{
	auto funName = func->getName();
	if (auto summary = globalState.getExternalTaintTable().lookup(funName))
		return evalCallBySummary(pp, func, *summary, evalResult);
	else
	{
		errs() << "Missing annotation for external function " << funName << "\n";
		llvm_unreachable("Please add annotation to the aforementioned function and in the config file and try again.\n");
	}
}

}