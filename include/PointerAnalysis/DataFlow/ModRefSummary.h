#ifndef TPA_MOD_REF_SUMMARY_H
#define TPA_MOD_REF_SUMMARY_H

#include "MemoryModel/Memory/Memory.h"
#include "Utils/VectorSet.h"

#include <llvm/ADT/iterator_range.h>

#include <unordered_map>

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

class ModRefSummary
{
private:
	using ValueSet = std::unordered_set<const llvm::Value*>;
	ValueSet valueReadSet;

	using MemSet = std::unordered_set<const MemoryLocation*>;
	MemSet memReadSet, memWriteSet;
public:
	using const_value_iterator = ValueSet::const_iterator;
	using const_mem_iterator = MemSet::const_iterator;

	bool addValueRead(const llvm::Value* val)
	{
		return valueReadSet.insert(val).second;
	}

	bool addMemoryRead(const MemoryLocation* loc)
	{
		return memReadSet.insert(loc).second;
	}

	template <typename Iterator>
	bool addMemoryRead(Iterator begin, Iterator end)
	{
		bool ret = false;
		for (auto itr = begin; itr != end; ++itr)
			ret |= addMemoryRead(*itr);
		return ret;
	}

	bool addMemoryWrite(const MemoryLocation* loc)
	{
		return memWriteSet.insert(loc).second;
	}

	template <typename Iterator>
	bool addMemoryWrite(Iterator begin, Iterator end)
	{
		bool ret = false;
		for (auto itr = begin; itr != end; ++itr)
			ret |= addMemoryWrite(*itr);
		return ret;
	}

	bool isValueRead(const llvm::Value* val) const
	{
		return valueReadSet.count(val);
	}

	bool isMemoryRead(const MemoryLocation* loc) const
	{
		return memReadSet.count(loc);
	}

	bool isMemoryWritten(const MemoryLocation* loc) const
	{
		return memWriteSet.count(loc);
	}

	llvm::iterator_range<const_value_iterator> value_reads() const
	{
		return llvm::iterator_range<const_value_iterator>(valueReadSet.begin(), valueReadSet.end());
	}
	llvm::iterator_range<const_mem_iterator> mem_reads() const
	{
		return llvm::iterator_range<const_mem_iterator>(memReadSet.begin(), memReadSet.end());
	}
	llvm::iterator_range<const_mem_iterator> mem_writes() const
	{
		return llvm::iterator_range<const_mem_iterator>(memWriteSet.begin(), memWriteSet.end());
	}

	bool isEmpty() const
	{
		return valueReadSet.empty() && memReadSet.empty() && memWriteSet.empty();
	}
};

class ModRefSummaryMap
{
private:
	using MapType = std::unordered_map<const llvm::Function*, ModRefSummary>;
	MapType summaryMap;
public:
	using const_iterator = MapType::const_iterator;

	ModRefSummary& getSummary(const llvm::Function* f)
	{
		return summaryMap[f];
	}
	const ModRefSummary& getSummary(const llvm::Function* f) const
	{
		auto itr = summaryMap.find(f);
		assert(itr != summaryMap.end());
		return itr->second;
	}

	const_iterator begin() const { return summaryMap.begin(); }
	const_iterator end() const { return summaryMap.end(); }

	void dump(llvm::raw_ostream& os) const;
};

}

#endif
