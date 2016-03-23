#pragma once

#include "Util/Hashing.h"
#include "Util/DataStructure/VectorSet.h"

#include <unordered_set>

namespace tpa
{

class MemoryObject;

class PtsSet
{
private:
	using SetType = util::VectorSet<const MemoryObject*>;
	const SetType* pSet;

	using PtsSetSet = std::unordered_set<SetType, util::ContainerHasher<SetType>>;
	static PtsSetSet existingSet;
	static const SetType* emptySet;

	PtsSet(const SetType* p): pSet(p) {}

	static const SetType* uniquifySet(SetType&& set);
public:
	using const_iterator = SetType::const_iterator;

	PtsSet insert(const MemoryObject*);
	PtsSet merge(const PtsSet&);

	bool has(const MemoryObject* obj) const
	{
		return pSet->count(obj);
	}
	bool includes(const PtsSet& rhs) const
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
	static PtsSet getSingletonSet(const MemoryObject*);
	static std::vector<const MemoryObject*> intersects(const PtsSet& s0, const PtsSet& s1);
	static PtsSet mergeAll(const std::vector<PtsSet>&);

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
