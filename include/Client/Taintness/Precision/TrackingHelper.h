#ifndef TPA_TAINT_TRACKING_HELPER_H
#define TPA_TAINT_TRACKING_HELPER_H

#include "Client/Lattice/TaintLattice.h"

#include <algorithm>

namespace client
{
namespace taint
{

template <typename IteratorType, typename LookupFunction, typename Action>
void propagateImprecision(IteratorType begin, IteratorType end, LookupFunction&& lookup, Action&& action, size_t sizeHint = 0)
{
	using ElemType = typename IteratorType::value_type;

	bool hasTaint = false, hasUntaint = false;
	std::vector<ElemType> candidates;
	if (sizeHint != 0)
		candidates.reserve(sizeHint);

	for (auto itr = begin; itr != end; ++itr)
	{
		TaintLattice taintVal = lookup(*itr);
		switch (taintVal)
		{
			case TaintLattice::Tainted:
				hasTaint = true;
				break;
			case TaintLattice::Untainted:
				hasUntaint = true;
				break;
			case TaintLattice::Either:
				candidates.push_back(*itr);
				break;
			case TaintLattice::Unknown:
				assert(false && "Unknown lattice found here?");
				break;
		}
	}

	// If we have both Taint and Untaint on the rhs, it's not useful to track any other operands even if they are imprecise, since the imprecision comes from path sensitivity
	if (!(hasTaint && hasUntaint))
		std::for_each(candidates.begin(), candidates.end(), std::forward<Action>(action));
}

template <typename ContainerType, typename LookupFunction, typename Action>
void propagateImprecision(const ContainerType& container, LookupFunction&& lookup, Action&& action)
{
	return propagateImprecision(container.begin(), container.end(), std::forward<LookupFunction>(lookup), std::forward<Action>(action), container.size());
}

}
}

#endif
