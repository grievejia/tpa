#ifndef TPA_VECTORSET_MANANGER_H
#define TPA_VECTORSET_MANANGER_H

#include "Utils/VectorSet.h"
#include "Utils/STLUtils.h"

#include <cassert>
#include <unordered_set>

namespace tpa
{

// This manager class guarantees that every VectorSet it creates must be unique
// It also provides interfaces for functional updates on VectorSet
template <typename ElemType>
class VectorSetManager
{
private:
	using VectorSetType = SortedVectorSet<ElemType>;

	const VectorSetType* emptySet;
	std::unordered_set<VectorSetType, ContainerHasher<VectorSetType>> existingSet;
public:
	VectorSetManager()
	{
		// Create emptyset first
		auto itr = existingSet.emplace().first;
		emptySet = &(*itr);
	}
	VectorSetManager(const VectorSetManager&) = delete;
	VectorSetManager(VectorSetManager&&) = delete;
	VectorSetManager& operator=(const VectorSetManager&) = delete;
	VectorSetManager& operator=(VectorSetManager&&) = delete;

	const VectorSetType* getEmptySet() const { return emptySet; }
	const VectorSetType* getSingletonSet(ElemType elem)
	{
		return insert(emptySet, elem);
	}

	const VectorSetType* insert(const VectorSetType* oldSet, ElemType elem)
	{
		assert(oldSet != nullptr);
		VectorSetType newSet(*oldSet);
		newSet.insert(elem);

		auto itr = existingSet.find(newSet);
		if (itr == existingSet.end())
			itr = existingSet.insert(itr, std::move(newSet));

		return &(*itr);
	}

	const VectorSetType* mergeSet(const VectorSetType* set1, const VectorSetType* set2)
	{
		assert(set1 != nullptr && set2 != nullptr);

		// The easy case
		if (set1 == set2)
			return set1;
		//else if (set1 == emptySet)
		//	return set2;
		//else if (set2 == emptySet)
		//	return set1;

		// We prefer to merge the smaller set into the larger set
		if (set1->getSize() < set2->getSize())
			std::swap(set1, set2);
		VectorSetType newSet(*set1);
		newSet.mergeWith(*set2);

		auto itr = existingSet.find(newSet);
		if (itr == existingSet.end())
			itr = existingSet.insert(itr, std::move(newSet));

		return &(*itr);
	}
};

}

#endif
