#include "MemoryModel/Memory/MemoryManager.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <limits>

using namespace llvm;

static cl::opt<bool> OutOfBoundWarning("oob-warning", cl::desc("Warn the user when out-of-bound pointer arithmetic is performed"), cl::init(false));

namespace tpa
{

bool MemoryObject::isGlobalObject() const
{
	auto inst = allocSite.getInstruction();
	return (inst == nullptr) || (isa<GlobalValue>(inst));
}

bool MemoryObject::isStackObject() const
{
	auto inst = allocSite.getInstruction();
	return (inst != nullptr) && (isa<AllocaInst>(inst));
}

bool MemoryObject::isHeapObject() const
{
	return !isGlobalObject() && !isStackObject();
}

MemoryManager::MemoryManager(DataLayout& d):
	universalObj(ProgramLocation(Context::getGlobalContext(), nullptr)),
	nullObj(ProgramLocation(Context::getGlobalContext(), nullptr)),
	dataLayout(d)
{
	universalObj.size = 1;
	universalObj.arrayLayout.push_back(MemoryObject::ArrayTriple{0, std::numeric_limits<size_t>::max(), 1});

	universalLoc = getMemoryLocation(&universalObj, 0, true);
	nullLoc = getMemoryLocation(&nullObj, 0, false);
}

const MemoryObject* MemoryManager::createMemoryObjectForFunction(const Function* f)
{
	auto fObj = allocateMemory(ProgramLocation(Context::getGlobalContext(), f), f->getType());
	objToFunction[fObj] = f;
	return fObj;
}

const Function* MemoryManager::getFunctionForObject(const MemoryObject* obj) const
{
	auto itr = objToFunction.find(obj);
	if (itr == objToFunction.end())
		return nullptr;
	else
		return itr->second;
}

size_t MemoryManager::initializeSummaryObject(MemoryObject& obj, Type* type)
{
	while (auto arrayType = dyn_cast<ArrayType>(type))
		type = arrayType->getElementType();
	while (auto vecType = dyn_cast<VectorType>(type))
		type = vecType->getElementType();

	auto typeSize = dataLayout.getTypeAllocSize(type);
	obj.arrayLayout.emplace_back(MemoryObject::ArrayTriple{0, std::numeric_limits<size_t>::max(), typeSize});
	return typeSize;
}

size_t MemoryManager::initializeMemoryObject(MemoryObject& obj, size_t offset, Type* type)
{
	if (type->isArrayTy() || type->isVectorTy())
	{
		auto seqType = cast<SequentialType>(type);
		auto elemType = seqType->getElementType();
		auto elemSize = dataLayout.getTypeAllocSize(elemType);
		auto elemCount = type->isArrayTy() ? cast<ArrayType>(type)->getNumElements() : cast<VectorType>(type)->getNumElements();
		if (elemCount > 1)
		{
			obj.arrayLayout.emplace_back(MemoryObject::ArrayTriple{offset, offset + elemSize * elemCount, elemSize});
		}
		initializeMemoryObject(obj, offset, elemType);
		return elemSize;
	}
	else if (auto stType = dyn_cast<StructType>(type))
	{
		size_t sumSize = 0;
		auto stLayout = dataLayout.getStructLayout(stType);
		for (auto i = 0u, e = stType->getNumElements(); i < e; ++i)
		{
			auto fieldOffset = stLayout->getElementOffset(i);
			sumSize += initializeMemoryObject(obj, offset + fieldOffset, stType->getElementType(i));
		}
		return sumSize;
	}
	else
		return dataLayout.getTypeAllocSize(type);
}

const MemoryObject* MemoryManager::allocateMemory(const ProgramLocation& loc, llvm::Type* type, bool forceSummary)
{
	auto itr = objPool.find(loc);
	if (itr != objPool.end())
		return &itr->second;

	itr = objPool.insert(itr, std::make_pair(loc, MemoryObject(loc)));
	auto& memObj = itr->second;

	auto totSize = forceSummary? initializeSummaryObject(memObj, type) : initializeMemoryObject(memObj, 0, type);
	assert(totSize != 0 && "allocating a size-0 object?");
	memObj.size = totSize;
	return &memObj;
}

void MemoryManager::setArgv(const MemoryLocation* ptrLoc, const MemoryLocation* memLoc)
{
	argvPtrLoc = ptrLoc;
	argvMemLoc = memLoc;
}

const MemoryLocation* MemoryManager::getMemoryLocation(const MemoryObject* obj, size_t offset, bool summary)
{
	assert(obj != nullptr);

	auto loc = MemoryLocation(obj, offset, summary);
	auto itr = locSet.insert(loc).first;
	assert(itr->isSummaryLocation() == summary);
	return &*itr;
}

const MemoryLocation* MemoryManager::getMemoryLocation(const MemoryObject* obj, size_t offset, bool summary) const
{
	assert(obj != nullptr);
	auto loc = MemoryLocation(obj, offset, summary);
	auto itr = locSet.find(loc);
	assert(itr != locSet.end());
	assert(itr->isSummaryLocation() == summary);
	return &*itr;
}

const MemoryLocation* MemoryManager::offsetMemory(const MemoryObject* obj, size_t offset)
{
	assert(obj != nullptr);

	if (obj == &universalObj || obj == &nullObj)
		return universalLoc;

	// Here we cannot just return MemoryLocation(obj, offset) because if obj is of a type that contains arrays, the offset may get messed up because of array collapsing. We have to adjust the offset according to the array layout of that particular memory object
	bool summary = false;
	for (auto const& arrayTriple: obj->arrayLayout)
	{
		if (arrayTriple.start > offset)
			break;

		if (arrayTriple.start <= offset && offset < arrayTriple.end)
		{
			summary = true;
			offset = arrayTriple.start + (offset - arrayTriple.start) % arrayTriple.size;
		}
	}

	if (offset > obj->size)
	{
		// Our intent here is to capture obvious out-of-bound pointer arithmetics. If such case is detected, return a universal object immediately, indicating that we have no idea where the out-of-bound pointer might points to
		// TODO: add a "strict mode" flag that, when set, will report an error instead of trying to carry on the analysis
		if (OutOfBoundWarning)
		{
			errs() << "Warning: out-of-bound pointer arithmetic\n";
			errs() << "\twhen computing " << *obj << " + " << offset << " (the bound is " << obj->size << ")\n";
		}
		return universalLoc;
	}

	return getMemoryLocation(obj, offset, summary);
}

const MemoryLocation* MemoryManager::offsetMemory(const MemoryObject* obj, size_t offset) const
{
	assert(obj != nullptr);

	if (obj == &universalObj || obj == &nullObj)
		return universalLoc;

	// Here we cannot just return MemoryLocation(obj, offset) because if obj is of a type that contains arrays, the offset may get messed up because of array collapsing. We have to adjust the offset according to the array layout of that particular memory object
	bool summary = false;
	for (auto const& arrayTriple: obj->arrayLayout)
	{
		if (arrayTriple.start > offset)
			break;

		if (arrayTriple.start <= offset && offset < arrayTriple.end)
		{
			summary = true;
			offset = arrayTriple.start + (offset - arrayTriple.start) % arrayTriple.size;
		}
	}

	if (offset > obj->size)
	{
		// Our intent here is to capture obvious out-of-bound pointer arithmetics. If such case is detected, return a universal object immediately, indicating that we have no idea where the out-of-bound pointer might points to
		// TODO: add a "strict mode" flag that, when set, will report an error instead of trying to carry on the analysis
		if (OutOfBoundWarning)
		{
			errs() << "Warning: out-of-bound pointer arithmetic\n";
			errs() << "\twhen computing " << *obj << " + " << offset << " (the bound is " << obj->size << ")\n";
		}
		return universalLoc;
	}

	return getMemoryLocation(obj, offset, summary);
}

const MemoryLocation* MemoryManager::offsetMemory(const MemoryLocation* loc, size_t offset)
{
	assert(loc != nullptr);
	if (offset == 0)
		return loc;
	else
	{
		// If we start from a summary object, then no matter how large the offset is, we should always end up within the object itself
		// If we start from a non-summary object, we should be able to "jump pass" the possible summary objects ahead of it
		auto locOffset = loc->getOffset();
		for (auto const& arrayTriple: loc->getMemoryObject()->arrayLayout)
		{
			if (arrayTriple.start <= locOffset && locOffset < arrayTriple.end)
				offset %= arrayTriple.size;
		}
		return offsetMemory(loc->getMemoryObject(), locOffset + offset);
	}
}

const MemoryLocation* MemoryManager::offsetMemory(const MemoryLocation* loc, size_t offset) const
{
	assert(loc != nullptr);
	if (offset == 0)
		return loc;
	else
	{
		// If we start from a summary object, then no matter how large the offset is, we should always end up within the object itself
		// If we start from a non-summary object, we should be able to "jump pass" the possible summary objects ahead of it
		auto locOffset = loc->getOffset();
		for (auto const& arrayTriple: loc->getMemoryObject()->arrayLayout)
		{
			if (arrayTriple.start <= locOffset && locOffset < arrayTriple.end)
				offset %= arrayTriple.size;
		}
		return offsetMemory(loc->getMemoryObject(), locOffset + offset);
	}
}

std::vector<const MemoryLocation*> MemoryManager::getAllOffsetLocations(const MemoryLocation* loc) const
{
	auto ret = std::vector<const MemoryLocation*>();

	auto locKey = *loc;
	auto itr = locSet.find(locKey);
	assert(itr != locSet.end());

	auto obj = loc->getMemoryObject();
	while (itr != locSet.end() && itr->getMemoryObject() == obj)
	{
		ret.push_back(&*itr);
		++itr;
	}

	return ret;
}

}
