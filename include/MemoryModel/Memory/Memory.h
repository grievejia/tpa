#ifndef TPA_MEMORY_OBJECT_H
#define TPA_MEMORY_OBJECT_H

#include "MemoryModel/Precision/ProgramLocation.h"

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

// MemoryObject is the object allocated by one allocation
class MemoryObject
{
private:
	ProgramLocation allocSite;
	size_t sz;

	struct ArrayTriple
	{
		size_t start, end, size;
	};

	std::vector<ArrayTriple> arrayLayout;

	MemoryObject(const ProgramLocation& loc): allocSite(loc), sz(0) {}
public:
	const ProgramLocation& getAllocationSite() const { return allocSite; }
	size_t size() const { return sz; }

	bool isGlobalObject() const;
	bool isStackObject() const;
	bool isHeapObject() const;

	friend class MemoryManager;
};

// MemoryLocation is conceptually a pair (MemoryObect, offset). It is the basic "unit" of our memory analysis.
class MemoryLocation
{
private:
	const MemoryObject* memObj;
	size_t offset;
	bool summary;

	MemoryLocation(const MemoryObject* m, size_t o, bool s): memObj(m), offset(o), summary(s) {}
public:
	const MemoryObject* getMemoryObject() const { return memObj; }
	size_t getOffset() const { return offset; }
	bool isSummaryLocation() const { return summary; }

	bool operator==(const MemoryLocation& other) const
	{
		return (memObj == other.memObj) && (offset == other.offset);
	}
	bool operator!=(const MemoryLocation& other) const
	{
		return !(*this == other);
	}
	bool operator<(const MemoryLocation& other) const
	{
		if (memObj != other.memObj)
			return memObj < other.memObj;
		else
			return offset < other.offset;
	}

	friend class MemoryManager;
};

llvm::raw_ostream& operator<<(llvm::raw_ostream&, const MemoryObject&);
llvm::raw_ostream& operator<<(llvm::raw_ostream&, const MemoryLocation&);

}

namespace std
{
	template<> struct hash<tpa::MemoryLocation>
	{
		size_t operator()(const tpa::MemoryLocation& loc) const
		{
			size_t seed = 0;
			tpa::hash_combine(seed, loc.getMemoryObject());
			tpa::hash_combine(seed, loc.getOffset());
			return seed;
		}
	};
}

#endif
