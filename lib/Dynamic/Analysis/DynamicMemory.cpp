#include "Dynamic/Analysis/DynamicContext.h"
#include "Dynamic/Analysis/DynamicMemory.h"

#include <cassert>

namespace dynamic
{

DynamicMemoryObject DynamicMemoryObject::getNullObject()
{
	return DynamicMemoryObject(DynamicPointer(DynamicContext::getGlobalContext(), 0), 0);
}

DynamicMemoryObject DynamicMemory::insertAllocMap(const DynamicPointer& ptr, const void* addr)
{
	auto itr = allocMap.find(addr);
	assert(itr == allocMap.end() && "Duplicate allocation!");
	allocMap.insert(itr, std::make_pair(addr, ptr));
	return DynamicMemoryObject(ptr, 0);
}

DynamicMemoryObject DynamicMemory::allocateGlobal(const DynamicPointer& ptr, const void* addr)
{
	return insertAllocMap(ptr, addr);
}

DynamicMemoryObject DynamicMemory::allocateStack(const DynamicPointer& ptr, const void* addr)
{
	stackCtxMap[ptr.getContext()].push_back(addr);
	return insertAllocMap(ptr, addr);
}

DynamicMemoryObject DynamicMemory::allocateHeap(const DynamicPointer& ptr, const void* addr)
{
	return insertAllocMap(ptr, addr);
}

DynamicMemoryObject DynamicMemory::allocate(AllocType type, const DynamicPointer& ptr, const void* addr)
{
	switch (type)
	{
		case AllocType::Global:
			return allocateGlobal(ptr, addr);
		case AllocType::Stack:
			return allocateStack(ptr, addr);
		case AllocType::Heap:
			return allocateHeap(ptr, addr);
	}
}

DynamicMemoryObject DynamicMemory::getMemoryObject(const void* addr)
{
	if (addr == nullptr)
		return DynamicMemoryObject::getNullObject();

	auto itr = allocMap.lower_bound(addr);
	if (itr == allocMap.end() || itr->first != addr)
		--itr;

	auto offset = static_cast<const char*>(addr) - static_cast<const char*>(itr->first);
	return DynamicMemoryObject(itr->second, offset);
}

void DynamicMemory::deallocateStack(const DynamicContext* ctx)
{
	auto itr = stackCtxMap.find(ctx);
	if (itr == stackCtxMap.end())
		return;
	for (auto addr: itr->second)
		allocMap.erase(addr);
	stackCtxMap.erase(itr);
}


}