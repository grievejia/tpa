#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"

namespace tpa
{

std::unordered_set<TypeLayout> TypeLayout::typeSet;

const TypeLayout* TypeLayout::getTypeLayout(size_t s, std::initializer_list<ArrayTriple> a, std::initializer_list<size_t> p)
{
	return TypeLayout::getTypeLayout(s, ArrayLayout::getLayout(std::move(a)), PointerLayout::getLayout(std::move(p)));
}

const TypeLayout* TypeLayout::getTypeLayout(size_t size, const ArrayLayout* a, const PointerLayout* p)
{
	assert(a != nullptr && p != nullptr);

	auto itr = typeSet.insert(TypeLayout(size, a, p)).first;
	return &(*itr);
}

const TypeLayout* TypeLayout::getPointerTypeLayoutWithSize(size_t size)
{
	return getTypeLayout(size, ArrayLayout::getDefaultLayout(), PointerLayout::getSinglePointerLayout());
}

const TypeLayout* TypeLayout::getNonPointerTypeLayoutWithSize(size_t size)
{
	return getTypeLayout(size, ArrayLayout::getDefaultLayout(), PointerLayout::getEmptyLayout());
}

const TypeLayout* TypeLayout::getByteArrayTypeLayout()
{
	return getTypeLayout(1, ArrayLayout::getByteArrayLayout(), PointerLayout::getSinglePointerLayout());
}

std::pair<size_t, bool> TypeLayout::offsetInto(size_t size) const
{
	return arrayLayout->offsetInto(size);
}

}