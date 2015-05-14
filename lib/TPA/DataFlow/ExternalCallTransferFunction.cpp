#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TransferFunction.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static bool isPtrArrayStructType(Type* type)
{
	return (isa<SequentialType>(type) || type->isStructTy());
}

static const Value* getArgument(const CallNode* callNode, const APosition& pos)
{
	auto inst = callNode->getInstruction();
	if (pos.isReturnPosition())
		return inst;

	ImmutableCallSite cs(inst);
	assert(cs);
	
	auto argIdx = pos.getAsArgPosition().getArgIndex();
	assert(cs.arg_size() > argIdx);

	return cs.getArgument(argIdx)->stripPointerCasts();
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
	auto mallocSize = (sizeVal == nullptr) ? 0 : getMallocSize(mallocType, sizeVal);

	auto sizeUnknown = (mallocSize == 0);
	auto objType = sizeUnknown ? mallocType : ArrayType::get(mallocType, mallocSize);

	return evalMemoryAllocation(dstPtr, objType, sizeUnknown);
}

EvalStatus TransferFunction::evalExternalAlloc(const CallNode* callNode, const PointerAllocEffect& allocEffect)
{
	if (allocEffect.hasSizePosition())
		return evalMallocWithSizeValue(callNode, getArgument(callNode, allocEffect.getSizePosition()));
	else
		return evalMallocWithSizeValue(callNode, nullptr);
}

EvalStatus TransferFunction::copyPointerPtsSet(const Pointer* dstPtr, const Pointer* srcPtr)
{
	auto srcSet = globalState.getEnv().lookup(srcPtr);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	auto envChanged = globalState.getEnv().weakUpdate(dstPtr, srcSet);
	return EvalStatus::getValidStatus(envChanged, false);
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
	{
		// Same as ReturnArg0
		auto effect = PointerEffect::getCopyEffect(CopyDest::getValue(APosition::getReturnPosition()), CopySource::getValue(APosition::getArgPosition(0)));
		return evalExternalCopy(callNode, effect.getAsCopyEffect());
	}
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
		if (globalState.getMemoryManager().isSpecialMemoryLocation(tgtLoc))
			break;
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

EvalStatus TransferFunction::evalMemcpy(const CallNode* callNode, const APosition& dstPos, const APosition& srcPos)
{
	assert(store != nullptr);
	assert(dstPos.isArgPosition() && srcPos.isArgPosition() && "memcpy only operates on arguments");

	auto dstPtr = getPointer(getArgument(callNode, dstPos));
	if (dstPtr == nullptr)
		return EvalStatus::getInvalidStatus();
	auto srcPtr = getPointer(getArgument(callNode, srcPos));
	if (srcPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	auto result = evalMemcpyPointer(dstPtr, srcPtr);
	return result;
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
			if (!memManager.isSpecialMemoryLocation(tgtLoc))
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

bool TransferFunction::setMemoryLocationTo(const MemoryLocation* loc, Type* type, PtsSet srcSet)
{
	bool changed = false;
	auto candidateLocs = findPointerCandidates(loc, type);

	for (auto loc: candidateLocs)
		changed |= store->weakUpdate(loc, srcSet);

	return changed;
}

EvalStatus TransferFunction::fillPtsSetWith(const Pointer* ptr, PtsSet srcSet)
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

		storeChanged |= setMemoryLocationTo(loc, elemType, srcSet);
	}

	return EvalStatus::getValidStatus(false, storeChanged);
}

EvalStatus TransferFunction::evalMemset(const CallNode* callNode)
{
	assert(callNode->getNumArgument() >= 1);
	assert(store != nullptr);

	ImmutableCallSite cs(callNode->getInstruction());
	assert(cs && cs.arg_size() >= 2);
	auto dstPtr = getPointer(callNode->getArgument(0));
	if (dstPtr == nullptr)
		return EvalStatus::getInvalidStatus();

	bool setToNull = false;
	if (auto cInt = dyn_cast<ConstantInt>(cs.getArgument(1)))
		setToNull = cInt->isZero();

	// If we memset a pointer to zero, then all the memory locations has to be updated to NULL; otherwise, we treat the memory as a non-pointer constant area
	if (!setToNull)
		return EvalStatus::getValidStatus(false, false);

	auto result = fillPtsSetWith(dstPtr, PtsSet::getSingletonSet(globalState.getMemoryManager().getNullLocation()));
	if (auto retVal = callNode->getDest())
	{
		auto retPtr = getOrCreatePointer(retVal);
		result = result || copyPointerPtsSet(retPtr, dstPtr);
	}
	return result;
}

PtsSet TransferFunction::evalExternalCopySource(const CallNode* callNode, const CopySource& src)
{
	switch (src.getType())
	{
		case CopySource::SourceType::Value:
		{
			auto ptr = getPointer(getArgument(callNode, src.getPosition()));
			if (ptr == nullptr)
				return PtsSet::getEmptySet();
			return globalState.getEnv().lookup(ptr);
		}
		case CopySource::SourceType::DirectMemory:
		{
			auto ptr = getPointer(getArgument(callNode, src.getPosition()));
			if (ptr == nullptr)
				return PtsSet::getEmptySet();
			return loadFromPointer(ptr);
		}
		case CopySource::SourceType::Universal:
		{
			return PtsSet::getSingletonSet(globalState.getMemoryManager().getUniversalLocation());
		}
		case CopySource::SourceType::Null:
		{
			return PtsSet::getSingletonSet(globalState.getMemoryManager().getNullLocation());
		}
		case CopySource::SourceType::Static:
			// TODO: model "static" memory
			return PtsSet::getSingletonSet(globalState.getMemoryManager().getUniversalLocation());
		case CopySource::SourceType::ReachableMemory:
			llvm_unreachable("ReachableMemory src should be handled earlier");
	}
}

EvalStatus TransferFunction::evalExternalCopyDest(const CallNode* callNode, const CopyDest& dest, PtsSet srcSet)
{
	// If the return value is not used, don't bother process it
	if (callNode->getDest() == nullptr && dest.getPosition().isReturnPosition())
		return EvalStatus::getValidStatus(false, false);

	auto dstPtr = getOrCreatePointer(getArgument(callNode, dest.getPosition()));
	switch (dest.getType())
	{
		case CopyDest::DestType::Value:
		{
			auto envChanged = globalState.getEnv().weakUpdate(dstPtr, srcSet);
			return EvalStatus::getValidStatus(envChanged, false);
		}
		case CopyDest::DestType::DirectMemory:
		{
			auto dstSet = globalState.getEnv().lookup(dstPtr);
			if (dstSet.isEmpty())
				return EvalStatus::getInvalidStatus();

			assert(store != nullptr);
			return weakUpdateStore(dstSet, srcSet);
		}
		case CopyDest::DestType::ReachableMemory:
			return fillPtsSetWith(dstPtr, srcSet);
	}
}

EvalStatus TransferFunction::evalExternalCopy(const CallNode* callNode, const PointerCopyEffect& copyEffect)
{
	auto const& src = copyEffect.getSource();
	auto const& dest = copyEffect.getDest();

	// Special case for memcpy: the source is not a single ptr/mem
	if (src.getType() == CopySource::SourceType::ReachableMemory)
	{
		assert(dest.getType() == CopyDest::DestType::ReachableMemory && "R src can only be assigned to R dest");
		return evalMemcpy(callNode, dest.getPosition(), src.getPosition());
	}

	auto srcSet = evalExternalCopySource(callNode, src);
	if (srcSet.isEmpty())
		return EvalStatus::getInvalidStatus();

	return evalExternalCopyDest(callNode, dest, srcSet);
}

EvalStatus TransferFunction::evalExternalCallByEffect(const CallNode* callNode, const PointerEffect& effect)
{
	switch (effect.getType())
	{
		case PointerEffectType::Alloc:
			return evalExternalAlloc(callNode, effect.getAsAllocEffect());
		case PointerEffectType::Copy:
			return evalExternalCopy(callNode, effect.getAsCopyEffect());
	}
}

EvalStatus TransferFunction::evalExternalCall(const CallNode* callNode, const Function* callee)
{
	auto summary = globalState.getExternalPointerTable().lookup(callee->getName());

	if (summary == nullptr)
	{
		errs() << "Unhandled external call: " << callee->getName() << "\n";
		llvm_unreachable("Consider annotate it in the config file");
	}

	EvalStatus retStatus = EvalStatus::getValidStatus(false, false);
	for (auto const& effect: *summary)
	{
		retStatus = retStatus || evalExternalCallByEffect(callNode, effect);
		if (!retStatus.isValid())
			break;
	}
	return retStatus;
}

}