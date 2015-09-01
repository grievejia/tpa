#include "PointerAnalysis/MemoryModel/Type/ArrayLayout.h"

namespace tpa
{

static bool validateTripleList(const ArrayLayout::ArrayTripleList& list)
{
	for (auto const& triple: list)
	{
		if (triple.start + triple.size > triple.end)
			return false;
		if ((triple.end - triple.start) % triple.size != 0)
			return false;
	}

	auto isSorted = std::is_sorted(
		list.begin(),
		list.end(),
		[] (auto const& lhs, auto const& rhs)
		{
			return
				(lhs.start < rhs.start) ||
				(lhs.start == rhs.start && lhs.size > rhs.size);
		}
	);

	if (!isSorted)
		return false;

	return std::unordered_set<ArrayTriple>(list.begin(), list.end()).size() == list.size();
}

const ArrayLayout* ArrayLayout::getLayout(ArrayTripleList&& list)
{
	assert(validateTripleList(list));
	auto itr = layoutSet.insert(ArrayLayout(std::move(list))).first;
	return &(*itr);
}

const ArrayLayout* ArrayLayout::getLayout(std::initializer_list<ArrayTriple> ilist)
{
	ArrayTripleList list(ilist);
	return getLayout(std::move(list));
}

const ArrayLayout* ArrayLayout::getByteArrayLayout()
{
	return getLayout({{0, std::numeric_limits<size_t>::max(), 1}});
}

const ArrayLayout* ArrayLayout::getDefaultLayout()
{
	return defaultLayout;
}

std::pair<size_t, bool> ArrayLayout::offsetInto(size_t offset) const
{
	bool hitArray = false;
	for (auto const& triple: arrayLayout)
	{
		if (triple.start > offset)
			break;

		if (triple.start <= offset && offset < triple.end)
		{
			hitArray = true;
			offset = triple.start + (offset - triple.start) % triple.size;
		}
	}
	return std::make_pair(offset, hitArray);
}

}