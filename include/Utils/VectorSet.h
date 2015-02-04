#ifndef TPA_VECTOR_SET_H
#define TPA_VECTOR_SET_H

#include <vector>
#include <algorithm>

namespace tpa
{

// Implementing set based on sorted std::vector.
// It is used primarily for representing points-to set
template <typename T, typename Compare = std::less<T>>
class VectorSet
{
public:
	// Export ElemType
	using ElemType = T;
	// Try to be consistent with STL containers
	using value_type = T;
private:
	// A sorted vector to represent ptsset
	using VectorSetType = std::vector<ElemType>;
	VectorSetType set;
public:
	using iterator = typename VectorSetType::iterator;
	using const_iterator = typename VectorSetType::const_iterator;

	VectorSet() = default;
	VectorSet(typename VectorSetType::size_type size): set(size) {}

	// Return true iff the set is changed
	bool insert(ElemType elem)
	{
		auto insPos = std::lower_bound(set.begin(), set.end(), elem, Compare());
		if (insPos == set.end() || elem < *insPos)
		{
			set.insert(insPos, elem);
			return true;
		}
		return false;
	}

	// Return true iff the set is changed
	bool erase(ElemType elem)
	{
		auto removePos = std::lower_bound(set.begin(), set.end(), elem);
		if (removePos == set.end() || elem < *removePos)
			return false;
		set.erase(removePos);
		return true;
	}

	// Return true iff the set is changed
	bool mergeWith(const VectorSet<T>& other)
	{
		bool changed = false;

		set.reserve(set.size() + other.set.size());
		for (auto const& elem: other)
		{
			if (!std::binary_search(set.begin(), set.end(), elem, Compare()))
			{
				changed = true;
				set.push_back(elem);
			}
		}
		if (changed)
			std::sort(set.begin(), set.end(), Compare());

		return changed;
	}

	// Return true iff the variable is found
	bool has(ElemType elem) const
	{
		return std::binary_search(set.begin(), set.end(), elem, Compare());
	}

	// Return true if *this is a superset of other
	bool contains(const VectorSet<T>& other) const
	{
		if (getSize() < other.getSize())
			return false;

		for (auto const& elem: other)
			if (!has(elem))
				return false;
		return true;
	}

	typename VectorSetType::size_type getSize() const
	{
		return set.size();
	}
	bool isEmpty() const
	{
		return set.empty();
	}

	bool operator==(const VectorSet<T>& other) const
	{
		return set == other.set;
	}
	bool operator!=(const VectorSet<T>& other) const
	{
		return !(*this == other);
	}

	bool intersects(const VectorSet<T>& other) const
	{
		VectorSetType intersectSet;
		if (this == &other)
			return true;
		std::set_intersection(begin(), end(), other.begin(), other.end(),
			std::inserter(intersectSet, intersectSet.begin()), Compare());
		return !intersectSet.empty();
	}

	// Iterators
	iterator begin() { return set.begin(); }
	iterator end() { return set.end(); }
	const_iterator begin() const { return set.begin(); }
	const_iterator end() const { return set.end(); }

	static std::vector<T> intersects(const VectorSet<T>& s0, const VectorSet<T>& s1)
	{
		std::vector<T> intersectSet;
		std::set_intersection(s0.begin(), s0.end(), s1.begin(), s1.end(),
			std::inserter(intersectSet, intersectSet.begin()), Compare());
		return intersectSet;
	}
};

}

#endif
