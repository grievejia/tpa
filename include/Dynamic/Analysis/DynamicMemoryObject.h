#pragma once

#include "Dynamic/Analysis/DynamicPointer.h"

#include <cstddef>

namespace dynamic
{

class DynamicMemoryObject
{
private:
	DynamicPointer allocSite;
	std::ptrdiff_t offset;
public:
	DynamicMemoryObject(const DynamicPointer& a, std::ptrdiff_t o): allocSite(a), offset(o) {}

	const DynamicPointer& getAllocSite() const { return allocSite; }
	std::ptrdiff_t getOffset() const { return offset; }

	static DynamicMemoryObject getNullObject();
};

}