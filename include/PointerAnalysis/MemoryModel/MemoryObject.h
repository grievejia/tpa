#pragma once

#include "PointerAnalysis/MemoryModel/MemoryBlock.h"
#include "Util/Hashing.h"

namespace tpa
{

// MemoryObject is conceptually a pair (MemoryBlock, offset). It is the unit of memory analysis.
class MemoryObject
{
private:
	const MemoryBlock* memBlock;
	size_t offset;
	bool summary;

	MemoryObject(const MemoryBlock* b, size_t o, bool s): memBlock(b), offset(o), summary(s)
	{
		assert(b != nullptr);
	}
public:
	const MemoryBlock* getMemoryBlock() const { return memBlock; }
	size_t getOffset() const { return offset; }
	bool isSummaryObject() const { return summary; }

	const AllocSite& getAllocSite() const { return memBlock->getAllocSite(); }

	bool isNullObject() const { return memBlock->isNullBlock(); }
	bool isUniversalObject() const { return memBlock->isUniversalBlock(); }
	bool isSpecialObject() const { return isNullObject() || isUniversalObject(); }
	
	bool isGlobalObject() const { return memBlock->isGlobalBlock(); }
	bool isFunctionObject() const { return memBlock->isFunctionBlock(); }
	bool isStackObject() const { return memBlock->isStackBlock(); }
	bool isHeapObject() const { return memBlock->isHeapBlock(); }

	bool operator==(const MemoryObject& other) const
	{
		return (memBlock == other.memBlock) && (offset == other.offset);
	}
	bool operator!=(const MemoryObject& other) const
	{
		return !(*this == other);
	}
	bool operator<(const MemoryObject& other) const
	{
		if (memBlock != other.memBlock)
			return memBlock < other.memBlock;
		else
			return offset < other.offset;
	}

	friend class MemoryManager;
};

}

namespace std
{
	template<> struct hash<tpa::MemoryObject>
	{
		size_t operator()(const tpa::MemoryObject& obj) const
		{
			return util::hashPair(obj.getMemoryBlock(), obj.getOffset());
		}
	};
}
