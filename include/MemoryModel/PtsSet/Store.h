#ifndef TPA_STORE_H
#define TPA_STORE_H

#include "MemoryModel/Memory/Memory.h"
#include "MemoryModel/PtsSet/PtsSetManager.h"

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

class Store
{
private:
	using MappingType = llvm::DenseMap<const MemoryLocation*, const PtsSet*>;

	MappingType mem;

	Store() = default;
public:
	using const_iterator = MappingType::const_iterator;

	const PtsSet* lookup(const MemoryLocation* loc) const
	{
		assert(loc != nullptr);
		auto itr = mem.find(loc);
		if (itr == mem.end())
			return nullptr;
		else
			return itr->second;
	}
	bool contains(const MemoryLocation* loc) const
	{
		return lookup(loc) != nullptr;
	}

	unsigned getSize() const { return mem.size(); }
	bool isEmpty() const { return mem.empty(); }

	bool operator==(const Store& other) const
	{
		if (getSize() != other.getSize())
			return false;
		for (auto const& mapping: other)
		{
			auto itr = mem.find(mapping.first);
			if (itr == mem.end())
				return false;
			if (itr->second != mapping.second)
				return false;
		}
		return true;
	}
	bool operator!=(const Store& other) const
	{
		return !(*this == other);
	}

	void dump(llvm::raw_ostream&) const;

	const_iterator begin() const { return mem.begin(); }
	const_iterator end() const { return mem.end(); }

	friend class StoreManager;
};

}

#endif
