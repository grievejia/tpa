#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"

using namespace context;

namespace tpa
{

static bool startWithSummary(const TypeLayout* type)
{
	bool ret;
	std::tie(std::ignore, ret) = type->offsetInto(0);
	return ret;
}

MemoryManager::MemoryManager(size_t pSize): ptrSize(pSize), argvObj(nullptr), envpObj(nullptr)
{
}

const MemoryObject* MemoryManager::getMemoryObject(const MemoryBlock* memBlock, size_t offset, bool summary) const
{
	assert(memBlock != nullptr);

	auto obj = MemoryObject(memBlock, offset, summary);
	auto itr = objSet.insert(obj).first;
	assert(itr->isSummaryObject() == summary);
	return &*itr;
}

const MemoryBlock* MemoryManager::allocateMemoryBlock(AllocSite allocSite, const TypeLayout* type)
{
	auto itr = allocMap.find(allocSite);
	if (itr == allocMap.end())
		itr = allocMap.insert(itr, std::make_pair(allocSite, MemoryBlock(allocSite, type)));
	assert(type == itr->second.getTypeLayout());
	return &itr->second;
}

const MemoryObject* MemoryManager::allocateGlobalMemory(const llvm::GlobalVariable* value, const TypeLayout* type)
{
	assert(value != nullptr && type != nullptr);

	auto memBlock = allocateMemoryBlock(AllocSite::getGlobalAllocSite(value), type);
	return getMemoryObject(memBlock, 0, startWithSummary(type));
}

const MemoryObject* MemoryManager::allocateMemoryForFunction(const llvm::Function* f)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getFunctionAllocSite(f), TypeLayout::getPointerTypeLayoutWithSize(0));
	return getMemoryObject(memBlock, 0, false);
}

const MemoryObject* MemoryManager::allocateStackMemory(const Context* ctx, const llvm::Value* ptr, const TypeLayout* type)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getStackAllocSite(ctx, ptr), type);
	return getMemoryObject(memBlock, 0, startWithSummary(type));
}

const MemoryObject* MemoryManager::allocateHeapMemory(const Context* ctx, const llvm::Value* ptr, const TypeLayout* type)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getHeapAllocSite(ctx, ptr), type);
	return getMemoryObject(memBlock, 0, true);
}

const MemoryObject* MemoryManager::allocateArgv(const llvm::Value* ptr)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getStackAllocSite(Context::getGlobalContext(), ptr), TypeLayout::getByteArrayTypeLayout());
	argvObj = getMemoryObject(memBlock, 0, true);
	return argvObj;
}

const MemoryObject* MemoryManager::allocateEnvp(const llvm::Value* ptr)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getStackAllocSite(Context::getGlobalContext(), ptr), TypeLayout::getByteArrayTypeLayout());
	envpObj = getMemoryObject(memBlock, 0, true);
	return envpObj;
}

const MemoryObject* MemoryManager::offsetMemory(const MemoryObject* obj, size_t offset) const
{
	assert(obj != nullptr);

	if (offset == 0)
		return obj;
	else
		return offsetMemory(obj->getMemoryBlock(), obj->getOffset() + offset);
}

const MemoryObject* MemoryManager::offsetMemory(const MemoryBlock* block, size_t offset) const
{
	assert(block != nullptr);

	if (block == &uBlock || block == &nBlock)
		return &uObj;

	auto type = block->getTypeLayout();

	bool summary;
	size_t adjustedOffset;
	std::tie(adjustedOffset, summary) = type->offsetInto(offset);
	// Heap objects are always summary
	summary = summary || block->isHeapBlock();

	if (adjustedOffset >= type->getSize())
		return &uObj;

	return getMemoryObject(block, adjustedOffset, summary);
}

std::vector<const MemoryObject*> MemoryManager::getReachablePointerObjects(const MemoryObject* obj, bool includeSelf) const
{
	auto ret = std::vector<const MemoryObject*>();
	if (includeSelf)
		ret.push_back(obj);

	if (!obj->isSpecialObject())
	{
		auto memBlock = obj->getMemoryBlock();
		auto ptrLayout = memBlock->getTypeLayout()->getPointerLayout();
		auto itr = ptrLayout->lower_bound(obj->getOffset());
		if (itr != ptrLayout->end() && *itr == obj->getOffset())
			++itr;
		std::transform(
			itr,
			ptrLayout->end(),
			std::back_inserter(ret),
			[this, memBlock] (size_t offset)
			{
				return offsetMemory(memBlock, offset);
			}
		);
	}

	return ret;
}

std::vector<const MemoryObject*> MemoryManager::getReachableMemoryObjects(const MemoryObject* obj) const
{
	auto ret = std::vector<const MemoryObject*>();

	if (obj->isSpecialObject())
	{
		ret.push_back(obj);
	}
	else
	{
		auto itr = objSet.find(*obj);
		assert(itr != objSet.end());

		auto block = obj->getMemoryBlock();
		while (itr != objSet.end() && itr->getMemoryBlock() == block)
		{
			ret.push_back(&*itr);
			++itr;
		}
	}

	return ret;
}

}