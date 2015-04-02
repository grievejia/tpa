#include "MemoryModel/PtsSet/PtsSet.h"

using namespace llvm;

namespace tpa
{

std::unordered_set<typename PtsSet::SetType, ContainerHasher<typename PtsSet::SetType>> PtsSet::existingSet;
const PtsSet::SetType* PtsSet::emptySet = &*(existingSet.insert(PtsSet::SetType()).first);

const PtsSet::SetType* PtsSet::uniquifySet(SetType&& set)
{
	auto itr = existingSet.find(set);
	if (itr == existingSet.end())
		itr = existingSet.insert(itr, std::move(set));

	return &*itr;
}

PtsSet PtsSet::insert(const MemoryLocation* loc)
{
	if (pSet->has(loc))
		return *this;
	
	SetType newSet(*pSet);
	newSet.insert(loc);

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

	// We prefer to merge the smaller set into the larger set
	auto set0 = pSet, set1 = rhs.pSet;
	if (set0->getSize() > set1->getSize())
		std::swap(set0, set1);
	SetType newSet(*set0);
	newSet.mergeWith(*set1);

	return PtsSet(uniquifySet(std::move(newSet)));
}

PtsSet PtsSet::getEmptySet()
{
	return PtsSet(emptySet);
}

PtsSet PtsSet::getSingletonSet(const MemoryLocation* loc)
{
	SetType newSet(loc);
	return PtsSet(uniquifySet(std::move(newSet)));
}

std::vector<const MemoryLocation*> PtsSet::intersects(const PtsSet& s0, const PtsSet& s1)
{
	return SetType::intersects(*s0.pSet, *s1.pSet);
}

}
