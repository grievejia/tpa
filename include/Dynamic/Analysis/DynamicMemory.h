#pragma once

#include "Dynamic/Analysis/DynamicMemoryObject.h"
#include "Dynamic/Instrument/AllocType.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace dynamic
{

class DynamicMemory
{
private:
	using AllocMapType = std::map<const void*, DynamicPointer>;
	AllocMapType allocMap;

	using ContextMapType = std::unordered_map<const DynamicContext*, std::vector<const void*>>;
	ContextMapType stackCtxMap;

	DynamicMemoryObject insertAllocMap(const DynamicPointer&, const void*);
	DynamicMemoryObject allocateGlobal(const DynamicPointer&, const void*);
	DynamicMemoryObject allocateStack(const DynamicPointer&, const void*);
	DynamicMemoryObject allocateHeap(const DynamicPointer&, const void*);
public:
	DynamicMemory() = default;

	DynamicMemoryObject allocate(AllocType type, const DynamicPointer&, const void*);
	DynamicMemoryObject getMemoryObject(const void*);
	void deallocateStack(const DynamicContext*);
};

}
