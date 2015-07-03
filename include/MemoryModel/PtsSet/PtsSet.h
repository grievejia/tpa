#ifndef TPA_PTS_SET_H
#define TPA_PTS_SET_H

#include "MemoryModel/Memory/Memory.h"
#include "Utils/VectorSet.h"
#include "Utils/Hashing.h"

#include <unordered_set>

namespace tpa
{

class PtsSet
{
private:
	using SetType = util::VectorSet<const MemoryLocation*>;
	const SetType* pSet;

	static std::unordered_set<SetType, ContainerHasher<SetType>>existingSet;
	static const SetType* emptySet;

	PtsSet() = delete;
	PtsSet(const SetType* p): pSet(p) {}

	static const SetType* uniquifySet(SetType&& set);
public:
	using const_iterator = SetType::const_iterator;

	PtsSet insert(const MemoryLocation* loc);
	PtsSet merge(const PtsSet& rhs);

	bool has(const MemoryLocation* loc) const
	{
		return pSet->count(loc);
	}
	bool contains(const PtsSet& rhs) const
	{
		return pSet->includes(*rhs.pSet);
	}

	bool empty() const
	{
		return pSet->empty();
	}
	size_t size() const
	{
		return pSet->size();
	}

	bool operator==(const PtsSet& rhs) const
	{
		return pSet == rhs.pSet;
	}
	bool operator!=(const PtsSet& rhs) const
	{
		return !(*this == rhs);
	}
	const_iterator begin() const { return pSet->begin(); }
	const_iterator end() const { return pSet->end(); }

	static PtsSet getEmptySet();
	static PtsSet getSingletonSet(const MemoryLocation* loc);
	static std::vector<const MemoryLocation*> intersects(const PtsSet& s0, const PtsSet& s1);

	friend std::hash<PtsSet>;
};

}

namespace std
{
	template<> struct hash<tpa::PtsSet>
	{
		size_t operator()(const tpa::PtsSet& p) const
		{
			return std::hash<const typename tpa::PtsSet::SetType*>()(p.pSet);
		}
	};
}

#endif
