#ifndef TPA_PTSSET_MANAGER_H
#define TPA_PTSSET_MANAGER_H

#include "MemoryModel/Memory/Memory.h"
#include "Utils/VectorSet.h"
#include "Utils/Hashing.h"

#include <unordered_set>

namespace tpa
{

using PtsSet = VectorSet<const MemoryLocation*>;

class PtsSetManager
{
private:
	const PtsSet* emptySet;
	std::unordered_set<PtsSet, ContainerHasher<PtsSet>> existingSet;
public:
	PtsSetManager()
	{
		// Create emptyset first
		auto itr = existingSet.insert(PtsSet()).first;
		emptySet = &(*itr);
	}
	PtsSetManager(const PtsSetManager&) = delete;
	PtsSetManager(PtsSetManager&&) = delete;
	PtsSetManager& operator=(const PtsSetManager&) = delete;
	PtsSetManager& operator=(PtsSetManager&&) = delete;

	const PtsSet* getEmptySet() const { return emptySet; }
	const PtsSet* getSingletonSet(const MemoryLocation* elem)
	{
		return insert(emptySet, elem);
	}

	const PtsSet* insert(const PtsSet* oldSet, const MemoryLocation* elem)
	{
		assert(oldSet != nullptr);
		PtsSet newSet(*oldSet);
		newSet.insert(elem);

		auto itr = existingSet.find(newSet);
		if (itr == existingSet.end())
			itr = existingSet.insert(itr, std::move(newSet));

		return &(*itr);
	}

	const PtsSet* mergeSet(const PtsSet* set1, const PtsSet* set2)
	{
		assert(set1 != nullptr && set2 != nullptr);

		// The easy case
		if (set1 == set2)
			return set1;
		else if (set1 == emptySet)
			return set2;
		else if (set2 == emptySet)
			return set1;

		// We prefer to merge the smaller set into the larger set
		if (set1->getSize() < set2->getSize())
			std::swap(set1, set2);
		PtsSet newSet(*set1);
		newSet.mergeWith(*set2);

		auto itr = existingSet.find(newSet);
		if (itr == existingSet.end())
			itr = existingSet.insert(itr, std::move(newSet));

		return &(*itr);
	}
};

}

#endif
