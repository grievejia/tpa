#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/Support/PtsSet.h"

namespace tpa
{

PtsSet::PtsSetSet PtsSet::existingSet;
const PtsSet::SetType* PtsSet::emptySet = &*(existingSet.insert(PtsSet::SetType()).first);

const PtsSet::SetType* PtsSet::uniquifySet(SetType&& set)
{
	if (set.count(MemoryManager::getUniversalObject()))
		set = { MemoryManager::getUniversalObject() };

	auto itr = existingSet.find(set);
	if (itr == existingSet.end())
	{
		set.shrink_to_fit();
		itr = existingSet.insert(itr, std::move(set));
	}

	return &*itr;
}

PtsSet PtsSet::insert(const MemoryObject* obj)
{
	if (pSet->count(obj))
		return *this;
	
	SetType newSet(*pSet);
	newSet.insert(obj);

	return PtsSet(uniquifySet(std::move(newSet)));
}

PtsSet PtsSet::merge(const PtsSet& rhs)
{
	// The easy case
	if (pSet == rhs.pSet)
		return *this;
	else if (pSet == emptySet)
		return rhs;
	else if (rhs.pSet == emptySet)
		return *this;

	SetType newSet(*pSet);
	newSet.merge(*rhs.pSet);
	return PtsSet(uniquifySet(std::move(newSet)));
}

PtsSet PtsSet::getEmptySet()
{
	return PtsSet(emptySet);
}

PtsSet PtsSet::getSingletonSet(const MemoryObject* obj)
{
	SetType newSet = { obj };
	return PtsSet(uniquifySet(std::move(newSet)));
}

std::vector<const MemoryObject*> PtsSet::intersects(const PtsSet& s0, const PtsSet& s1)
{
	return SetType::intersects(*s0.pSet, *s1.pSet);
}

PtsSet PtsSet::mergeAll(const std::vector<PtsSet>& sets)
{
	size_t totSize = 0;
	for (auto const& pSet: sets)
		totSize += pSet.size();

	SetType flatSet;
	flatSet.reserve(totSize);

	for (auto const& pSet: sets)
		flatSet.merge(*pSet.pSet);

	return uniquifySet(SetType(std::move(flatSet)));
}

}