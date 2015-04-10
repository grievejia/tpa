#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TransferFunction.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static inline size_t countPointerArguments(const Function* f)
{
	size_t ret = 0;
	for (auto& arg: f->args())
	{
		if (arg.getType()->isPointerTy())
			++ret;
	}
	return ret;
};

TransferFunction::TransferFunction(const Context* c, SemiSparseGlobalState& g): ctx(c), store(nullptr), globalState(g) {}
TransferFunction::TransferFunction(const Context* c, Store& s, SemiSparseGlobalState& g): ctx(c), store(&s), globalState(g) {}

// Return NULL if no pointer found
const Pointer* TransferFunction::getPointer(const Value* val)
{
	if (isa<GlobalValue>(val))
		return globalState.getPointerManager().getPointer(Context::getGlobalContext(), val);
	else
		return globalState.getPointerManager().getPointer(ctx, val);
}

const Pointer* TransferFunction::getOrCreatePointer(const Value* val)
{
	return globalState.getPointerManager().getOrCreatePointer(ctx, val);
}

const Pointer* TransferFunction::getOrCreatePointer(const Context* newCtx, const Value* val)
{
	return globalState.getPointerManager().getOrCreatePointer(newCtx, val);
}

EvalStatus TransferFunction::evalMemoryAllocation(const Pointer* dstPtr, Type* allocType, bool summary)
{
	auto memObj = globalState.getMemoryManager().allocateMemory(ProgramLocation(ctx, dstPtr->getValue()), allocType, summary);
	auto memLoc = globalState.getMemoryManager().offsetMemory(memObj, 0);

	auto envChanged = globalState.getEnv().insert(dstPtr, memLoc);
	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TransferFunction::evalAllocNode(const AllocNode* allocNode)
{
	auto allocInst = allocNode->getDest();
	assert(isa<AllocaInst>(allocInst));
	auto ptr = getOrCreatePointer(allocInst);

	return evalMemoryAllocation(ptr, allocNode->getAllocType(), false);
}

EvalStatus TransferFunction::evalCopyNodeWithZeroOffset(const CopyNode* copyNode)
{
	assert(copyNode->getOffset() == 0 && copyNode->getNumSrc() > 0);

	auto resSet = PtsSet::getEmptySet();
	for (auto src: *copyNode)
	{
		auto srcPtr = getPointer(src);
		if (srcPtr == nullptr)
			return EvalStatus::getInvalidStatus();

		auto pSet = globalState.getEnv().lookup(srcPtr);
		if (pSet.isEmpty())
			return EvalStatus::getInvalidStatus();

		resSet = resSet.merge(pSet);
	}

	auto dstPtr = getOrCreatePointer(copyNode->getDest());
	auto envChanged = globalState.getEnv().strongUpdate(dstPtr, resSet);
	return EvalStatus::getValidStatus(envChanged, false);
}

PtsSet TransferFunction::updateOffsetLocation(PtsSet pSet, const MemoryLocation* srcLoc, size_t offset, bool isArrayRef)
{
	auto resSet = pSet;
	// We have two cases here:
	// - For non-array reference, just access the variable with the given offset
	// - For array reference, we have to examine the variables with offset * 0, offset * 1, offset * 2... all the way till the memory region boundary, if the memory object is not known to be an array previously (this may happen if the program contains nonarray-to-array bitcast)
	if (isArrayRef && !srcLoc->isSummaryLocation())
	{
		auto objSize = srcLoc->getMemoryObject()->getSize();

		for (unsigned i = 0, e = objSize - srcLoc->getOffset(); i < e; i += offset)
		{
			auto offsetLoc = globalState.getMemoryManager().offsetMemory(srcLoc, i);
			resSet = resSet.insert(offsetLoc);
		}
	}
	else
	{
		auto offsetLoc = globalState.getMemoryManager().offsetMemory(srcLoc, offset);
		resSet = resSet.insert(offsetLoc);
	}
	return resSet;
}

EvalStatus TransferFunction::copyWithOffset(const Pointer* dst, const Pointer* src, size_t offset, bool isArrayRef)
{
	auto srcSet = globalState.getEnv().lookup(src);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto resSet = PtsSet::getEmptySet();
	for (auto srcLoc: srcSet)
	{
		if (globalState.getMemoryManager().isSpecialMemoryLocation(srcLoc))
		{
			// For unknown location, we are unable to track its points-to set
			// For null location, doing pointer arithmetic on it result in undefined behavior
			// TODO: report this to the user
			continue;
		}

		resSet = updateOffsetLocation(resSet, srcLoc, offset, isArrayRef);
	}

	auto envChanged = globalState.getEnv().strongUpdate(dst, resSet);
	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TransferFunction::evalCopyNodeWithNonZeroOffset(const CopyNode* copyNode)
{
	assert(copyNode->getOffset() != 0 && copyNode->getNumSrc() == 1);

	auto srcPtr = getPointer(copyNode->getFirstSrc());
	auto dstPtr = getOrCreatePointer(copyNode->getDest());
	if (srcPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	return copyWithOffset(dstPtr, srcPtr, copyNode->getOffset(), copyNode->isArrayRef());
}

EvalStatus TransferFunction::evalCopyNode(const CopyNode* copyNode)
{
	if (copyNode->getOffset() == 0)
		return evalCopyNodeWithZeroOffset(copyNode);
	else
		return evalCopyNodeWithNonZeroOffset(copyNode);
}

EvalStatus TransferFunction::evalLoadNode(const LoadNode* loadNode)
{
	assert(store != nullptr);

	auto srcPtr = getPointer(loadNode->getSrc());
	assert(srcPtr != nullptr && "LoadNode is evaluated before its src operand becomes available");
	auto dstPtr = getOrCreatePointer(loadNode->getDest());

	auto srcSet = globalState.getEnv().lookup(srcPtr);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto resSet = PtsSet::getEmptySet();
	for (auto tgtLoc: srcSet)
	{
		auto tgtSet = store->lookup(tgtLoc);
		if (!tgtSet.isEmpty())
			resSet = resSet.merge(tgtSet);
	}

	auto envChanged = globalState.getEnv().strongUpdate(dstPtr, resSet);
	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TransferFunction::strongUpdateStore(const MemoryLocation* dstLoc, PtsSet srcSet)
{
	auto storeChanged = store->strongUpdate(dstLoc, srcSet);
	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TransferFunction::weakUpdateStore(PtsSet dstSet, PtsSet srcSet)
{
	// Weak update
	bool storeChanged = false;
	for (auto updateLoc: dstSet)
	{
		if (globalState.getMemoryManager().isSpecialMemoryLocation(updateLoc))
		{
			// We won't process stores into unknown/null location
			// This is an kind of unsound behavior
			// TODO: report it to the user
			continue;
		}
		storeChanged |= store->weakUpdate(updateLoc, srcSet);
	}
	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TransferFunction::evalStore(const Pointer* dst, const Pointer* src)
{
	auto srcSet = globalState.getEnv().lookup(src);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto dstSet = globalState.getEnv().lookup(dst);
	if (dstSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto dstLoc = *dstSet.begin();
	// If the store target is precise and the target location is not unknown/null
	if (dstSet.getSize() == 1 && !dstLoc->isSummaryLocation())
		return strongUpdateStore(dstLoc, srcSet);
	return weakUpdateStore(dstSet, srcSet);
}

EvalStatus TransferFunction::evalStoreNode(const StoreNode* storeNode)
{
	assert(store != nullptr);

	auto srcPtr = getPointer(storeNode->getSrc());
	auto dstPtr = getPointer(storeNode->getDest());
	if (srcPtr == nullptr || dstPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	return evalStore(dstPtr, srcPtr);
}

std::vector<const llvm::Function*> TransferFunction::findFunctionsInPtsSet(PtsSet pSet, const CallNode* callNode)
{
	auto callees = std::vector<const Function*>();

	// Two cases here
	if (pSet.has(globalState.getMemoryManager().getUniversalLocation()))
	{
		// If funSet contains unknown location, then we can't really derive callees based on the points-to set
		// Instead, guess callees based on the number of arguments
		auto defaultTargets = globalState.getProgram().at_funs();
		std::copy_if(
			defaultTargets.begin(),
			defaultTargets.end(),
			std::back_inserter(callees),
			[callNode] (const Function* f)
			{
				bool isArgMatch = f->isVarArg() || countPointerArguments(f) == callNode->getNumArgument();
				bool isRetMatch = (f->getReturnType()->isPointerTy()) != (callNode->getDest() == nullptr);
				return isArgMatch && isRetMatch;
			}
		);
	}
	else
	{
		for (auto loc: pSet)
		{
			if (auto tgtFunc = globalState.getMemoryManager().getFunctionForObject(loc->getMemoryObject()))
				callees.push_back(tgtFunc);
		}
	}

	return callees;
}

std::vector<PtsSet> TransferFunction::collectArgumentPtsSets(const CallNode* callNode, size_t numUsefulArgs)
{
	std::vector<PtsSet> result;
	result.reserve(numUsefulArgs);
	
	auto argItr = callNode->begin();
	for (auto i = 0u; i < numUsefulArgs; ++i)
	{
		auto argPtr = getPointer(*argItr);
		if (argPtr == nullptr)
			break;

		auto pSet = globalState.getEnv().lookup(argPtr);
		if (pSet.isEmpty())
			break;

		result.push_back(pSet);
		++argItr;
	}

	return result;
}

bool TransferFunction::updateParameterPtsSets(const Context* newCtx, const PointerCFG& cfg, const std::vector<PtsSet>& argSets)
{
	auto changed = false;

	auto paramItr = cfg.param_begin();
	auto getNextPointerParamValue = [&paramItr, &cfg] ()
	{
		while (!paramItr->getType()->isPointerTy() && paramItr != cfg.param_end())
			++paramItr;
		auto retVal = paramItr;
		++paramItr;
		return retVal;
	};

	for (auto pSet: argSets)
	{
		auto paramVal = getNextPointerParamValue();
		auto paramPtr = getOrCreatePointer(newCtx, paramVal);
		changed |= globalState.getEnv().weakUpdate(paramPtr, pSet);
	}

	return changed;
}

EvalStatus TransferFunction::evalCallArguments(const CallNode* callNode, const Context* newCtx, const Function* callee)
{
	// Some codes do weird things like casting a 0-ary function into a vararg function and call it. We couldn't make any strict equality assumption about the number of arguments here except that the caller must provide enough actuals to make all the callee's formals well-defined
	auto numPtsArguments = countPointerArguments(callee);
	assert(callNode->getNumArgument() >= numPtsArguments);

	auto argSets = collectArgumentPtsSets(callNode, numPtsArguments);
	if (argSets.size() < numPtsArguments)
		return EvalStatus::getInvalidStatus();

	auto tgtCFG = globalState.getProgram().getPointerCFG(callee);
	assert(tgtCFG != nullptr);
	auto envChanged = updateParameterPtsSets(newCtx, *tgtCFG, argSets);

	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TransferFunction::evalReturnValue(const ReturnNode* retNode, const Context* oldCtx, const CallNode* oldNode)
{
	auto retVal = retNode->getReturnValue();
	if (retVal == nullptr)
	{
		assert(oldNode->getDest() == nullptr && "return an unexpected void");
		return EvalStatus::getValidStatus(false, false);
	}

	// Returned a value, but not used by the caller
	auto dstVal = oldNode->getDest();
	if (dstVal == nullptr)
		return EvalStatus::getValidStatus(false, false);

	auto retPtr = getPointer(retVal);
	if (retPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	auto retSet = globalState.getEnv().lookup(retPtr);
	if (retSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto dstPtr = getOrCreatePointer(oldCtx, dstVal);
	auto envChanged = globalState.getEnv().weakUpdate(dstPtr, retSet);
	return EvalStatus::getValidStatus(envChanged, false);
}

std::vector<const llvm::Function*> TransferFunction::resolveCallTarget(const CallNode* callNode)
{
	auto callees = std::vector<const Function*>();

	auto funPtr = getPointer(callNode->getFunctionPointer());
	if (funPtr != nullptr)
	{
		auto funSet = globalState.getEnv().lookup(funPtr);
		if (!funSet.isEmpty())
			callees = findFunctionsInPtsSet(funSet, callNode);
	}

	return callees;
}

}