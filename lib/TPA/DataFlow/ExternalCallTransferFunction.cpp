#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TransferFunction.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>

using namespace llvm;

namespace tpa
{

static bool isPtrArrayStructType(Type* type)
{
	return (isa<SequentialType>(type) || type->isStructTy());
}

static PointerType* getMallocType(const CallNode* callNode)
{
	auto callInst = callNode->getInstruction();
	PointerType* mallocType = nullptr;
	size_t numOfBitCastUses = 0;

	// Determine if CallInst has a bitcast use.
	for (auto user: callInst->users())
	{
		if (const BitCastInst* bcInst = dyn_cast<BitCastInst>(user))
		{
			mallocType = cast<PointerType>(bcInst->getDestTy());
			numOfBitCastUses++;
		}
	}

	// Malloc call has 1 bitcast use, so type is the bitcast's destination type.
	if (numOfBitCastUses == 1)
		return mallocType;

	// Malloc call was not bitcast, so type is the malloc function's return type.
	if (numOfBitCastUses == 0)
		return cast<PointerType>(callInst->getType());

	// Type could not be determined. Return i8* as a conservative answer
	return Type::getInt8PtrTy(callInst->getContext());
}

// Return 0 if the size cannot be statically determined
size_t TransferFunction::getMallocSize(Type* mallocType, const Value* sizeVal)
{
	auto mallocSize = 0u;
	if (auto cInt = dyn_cast<ConstantInt>(sizeVal))
		mallocSize = cInt->getZExtValue() / (globalState.getMemoryManager().getDataLayout().getTypeAllocSize(mallocType));

	return mallocSize;
}

EvalStatus TransferFunction::evalMallocWithSizeValue(const CallNode* callNode, const Value* sizeVal)
{
	// If the return value is not used, don't bother process it
	auto dstVal = callNode->getDest();
	if (dstVal == nullptr)
		return EvalStatus::getValidStatus(false, false);
	auto dstPtr = getOrCreatePointer(dstVal);

	auto mallocType = getMallocType(callNode)->getPointerElementType();
	auto mallocSize = getMallocSize(mallocType, sizeVal);

	auto sizeUnknown = (mallocSize == 0);
	auto objType = sizeUnknown ? mallocType : ArrayType::get(mallocType, mallocSize);

	return evalMemoryAllocation(dstPtr, objType, sizeUnknown);
}

EvalStatus TransferFunction::evalMalloc(const CallNode* callNode)
{
	ImmutableCallSite cs(callNode->getInstruction());
	auto sizeVal = cs.getArgument(0);
	return evalMallocWithSizeValue(callNode, sizeVal);
}

EvalStatus TransferFunction::copyPointerPtsSet(const Pointer* dstPtr, const Pointer* srcPtr)
{
	auto srcSet = globalState.getEnv().lookup(srcPtr);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto envChanged = globalState.getEnv().weakUpdate(dstPtr, srcSet);
	return EvalStatus::getValidStatus(envChanged, false);
}

EvalStatus TransferFunction::evalExternalReturnsArg(const CallNode* callNode, size_t argNum)
{
	assert(callNode->getNumArgument() > argNum);

	// If the return value is not used, don't bother process it
	auto dstVal = callNode->getDest();
	if (dstVal == nullptr)
		return EvalStatus::getValidStatus(false, false);

	auto srcVal = callNode->getArgument(argNum);
	auto srcPtr = getPointer(srcVal);
	if (srcPtr == nullptr)
		return EvalStatus::getValidStatus(false, false);
	auto dstPtr = getOrCreatePointer(dstVal);
	
	return copyPointerPtsSet(dstPtr, srcPtr);
}

EvalStatus TransferFunction::evalRealloc(const CallNode* callNode)
{
	ImmutableCallSite cs(callNode->getInstruction());

	auto ptrVal = cs.getArgument(0);
	auto sizeVal = cs.getArgument(1);

	if (isa<ConstantPointerNull>(ptrVal))
		// Same as malloc
		return evalMallocWithSizeValue(callNode, sizeVal);
	else
		// Same as ReturnArg0
		return evalExternalReturnsArg(callNode, 0);
}

EvalStatus TransferFunction::evalExternalReturnsStatic(const CallNode* callNode)
{
	// We can, of course, create a special variable which represents all the static memory locations
	// For now, however, just return a universal pointer
	// If the return value is not used, don't bother process it
	auto dstVal = callNode->getDest();
	if (dstVal == nullptr)
		return EvalStatus::getValidStatus(false, false);
	auto dstPtr = getOrCreatePointer(dstVal);
	
	return copyPointerPtsSet(dstPtr, globalState.getPointerManager().getUniversalPtr());
}

EvalStatus TransferFunction::evalExternalStore(const CallNode* callNode, size_t dstNum, size_t srcNum)
{
	assert(callNode->getNumArgument() > dstNum && callNode->getNumArgument() > srcNum);
	assert(store != nullptr);

	auto dstPtr = getPointer(callNode->getArgument(dstNum));
	if (dstPtr == nullptr)
		return EvalStatus::getInvalidStatus();
	auto srcPtr = getPointer(callNode->getArgument(srcNum));
	if (srcPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	return evalStore(dstPtr, srcPtr);
}

bool TransferFunction::copyMemoryPtsSet(const MemoryLocation* dstLoc, const std::vector<const MemoryLocation*>& srcLocs, size_t startingOffset)
{
	bool changed = false;
	for (auto oLoc: srcLocs)
	{
		auto oSet = store->lookup(oLoc);
		if (oSet.isEmpty())
			continue;

		auto offset = oLoc->getOffset() - startingOffset;
		auto tgtLoc = globalState.getMemoryManager().offsetMemory(dstLoc, offset);
		changed |= store->weakUpdate(tgtLoc, oSet);
	}
	return changed;
}

EvalStatus TransferFunction::evalMemcpyPointer(const Pointer* dstPtr, const Pointer* srcPtr)
{
	auto dstSet = globalState.getEnv().lookup(dstPtr);
	if (dstSet.isEmpty())
		return EvalStatus::getInvalidStatus();
	auto srcSet = globalState.getEnv().lookup(srcPtr);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	bool storeChanged = false;
	for (auto srcLoc: srcSet)
	{
		auto srcLocs = globalState.getMemoryManager().getAllOffsetLocations(srcLoc);
		for (auto dstLoc: dstSet)
			storeChanged |= copyMemoryPtsSet(dstLoc, srcLocs, srcLoc->getOffset());
	}

	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TransferFunction::evalMemcpy(const CallNode* callNode, size_t dstNum, size_t srcNum)
{
	assert(callNode->getNumArgument() > dstNum && callNode->getNumArgument() > srcNum);
	assert(store != nullptr);

	auto dstPtr = getPointer(callNode->getArgument(dstNum));
	if (dstPtr == nullptr)
		return EvalStatus::getInvalidStatus();
	auto srcPtr = getPointer(callNode->getArgument(srcNum));
	if (srcPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	auto memcpyRes = evalMemcpyPointer(dstPtr, srcPtr);
	if (!memcpyRes.isValid())
		return memcpyRes;
	else
	{
		bool envChanged = false;
		if (auto retVal = callNode->getDest())
		{
			auto retPtr = getOrCreatePointer(retVal);
			auto ptrRes = copyPointerPtsSet(retPtr, dstPtr);
			envChanged = ptrRes.hasEnvChanged();	
		}	
		return EvalStatus::getValidStatus(envChanged, memcpyRes.hasStoreChanged());
	}
}

std::vector<const MemoryLocation*> TransferFunction::findPointerCandidatesInStruct(const MemoryLocation* loc, StructType* stType, size_t baseOffset)
{
	std::vector<const MemoryLocation*> candidates;

	auto& memManager = globalState.getMemoryManager();
	auto stLayout = memManager.getDataLayout().getStructLayout(stType);
	for (unsigned i = 0, e = stType->getNumElements(); i != e; ++i)
	{
		auto elemType = stType->getElementType(i);
		unsigned offset = stLayout->getElementOffset(i);

		while (elemType->isArrayTy())
			elemType = elemType->getArrayElementType();

		if (elemType->isPointerTy())
		{
			auto tgtLoc = memManager.offsetMemory(loc, baseOffset + offset);
			if (!isSpecialLocation(tgtLoc))
				candidates.push_back(tgtLoc);
		}
		else if (auto subStType = dyn_cast<StructType>(elemType))
			findPointerCandidatesInStruct(loc, subStType, baseOffset + offset);
	}

	return candidates;
}

std::vector<const MemoryLocation*> TransferFunction::findPointerCandidates(const MemoryLocation* loc, Type* type)
{
	std::vector<const MemoryLocation*> candidates;

	while (auto arrayType = dyn_cast<ArrayType>(type))
		type = arrayType->getElementType();

	if (type->isPointerTy())
		candidates.push_back(loc);
	else if (auto stType = dyn_cast<StructType>(type))
		candidates = findPointerCandidatesInStruct(loc, stType, 0);

	return candidates;
}

bool TransferFunction::setMemoryLocationToNull(const MemoryLocation* loc, Type* type)
{
	bool changed = false;
	auto candidateLocs = findPointerCandidates(loc, type);

	auto nullSet = PtsSet::getSingletonSet(globalState.getMemoryManager().getNullLocation());
	for (auto loc: candidateLocs)
		changed |= store->weakUpdate(loc, nullSet);

	return changed;
}

EvalStatus TransferFunction::fillPtsSetWithNull(const Pointer* ptr)
{
	// If the pointer points to a non-pointer, non-array and non-struct memory, just ignore it
	auto elemType = ptr->getValue()->stripPointerCasts()->getType()->getPointerElementType();
	if (!isPtrArrayStructType(elemType))
		return EvalStatus::getValidStatus(false, false);

	auto pSet = globalState.getEnv().lookup(ptr);
	if (pSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto storeChanged = false;
	for (auto loc: pSet)
	{
		if (loc == globalState.getMemoryManager().getUniversalLocation())
			continue;

		storeChanged |= setMemoryLocationToNull(loc, elemType);
	}

	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TransferFunction::evalMemset(const CallNode* callNode)
{
	assert(callNode->getNumArgument() >= 1);
	assert(store != nullptr);

	ImmutableCallSite cs(callNode->getInstruction());
	assert(cs && cs.arg_size() >= 2);
	auto dstPtr = getPointer(cs.getArgument(0));
	if (dstPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	bool setToNull = false;
	if (auto cInt = dyn_cast<ConstantInt>(cs.getArgument(1)))
		setToNull = cInt->isZero();

	// If we memset a pointer to zero, then all the memory locations has to be updated to NULL; otherwise, we treat the memory as a non-pointer constant area
	if (!setToNull)
		return EvalStatus::getValidStatus(false, false);

	auto memRes = fillPtsSetWithNull(dstPtr);
	if (!memRes.isValid())
		return memRes;
	else
	{
		bool envChanged = false;
		if (auto retVal = callNode->getDest())
		{
			auto retPtr = getOrCreatePointer(retVal);
			auto ptrRes = copyPointerPtsSet(retPtr, dstPtr);
			envChanged = ptrRes.hasEnvChanged();	
		}	
		return EvalStatus::getValidStatus(envChanged, memRes.hasStoreChanged());
	}
}

EvalStatus TransferFunction::evalExternalCall(const CallNode* callNode, const Function* callee)
{
	auto extType = globalState.getExternalPointerEffectTable().lookup(callee->getName());

	switch (extType)
	{
		case PointerEffect::NoEffect:
			// Do nothing
			return EvalStatus::getValidStatus(false, false);
		case PointerEffect::Realloc:
			return evalRealloc(callNode);
		case PointerEffect::Malloc:
			return evalMalloc(callNode);
		case PointerEffect::ReturnArg0:
			return evalExternalReturnsArg(callNode, 0);
		case PointerEffect::ReturnArg1:
			return evalExternalReturnsArg(callNode, 1);
		case PointerEffect::ReturnArg2:
			return evalExternalReturnsArg(callNode, 2);
		case PointerEffect::ReturnStatic:
			return evalExternalReturnsStatic(callNode);
		case PointerEffect::StoreArg0ToArg1:
			return evalExternalStore(callNode, 1, 0);
		case PointerEffect::MemcpyArg1ToArg0:
			return evalMemcpy(callNode, 0, 1);
		case PointerEffect::Memset:
			return evalMemset(callNode);
		default:
			llvm_unreachable("Unhandled external call");
	}
}

}