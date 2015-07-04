#include "PointerAnalysis/MemoryModel/Type/PointerLayout.h"

namespace tpa
{

std::unordered_set<PointerLayout> PointerLayout::layoutSet;
const PointerLayout* PointerLayout::emptyLayout = PointerLayout::getLayout(util::VectorSet<size_t>());
const PointerLayout* PointerLayout::singlePointerLayout = PointerLayout::getLayout({0});

const PointerLayout* PointerLayout::getEmptyLayout()
{
	return emptyLayout;
}

const PointerLayout* PointerLayout::getSinglePointerLayout()
{
	return singlePointerLayout;
}

const PointerLayout* PointerLayout::getLayout(SetType&& set)
{
	auto itr = layoutSet.insert(PointerLayout(std::move(set))).first;
	return &(*itr);
}

const PointerLayout* PointerLayout::getLayout(std::initializer_list<size_t> ilist)
{
	SetType set(ilist);
	return getLayout(std::move(set));
}

}