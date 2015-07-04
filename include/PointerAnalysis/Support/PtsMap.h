#pragma once

#include "PointerAnalysis/Support/PtsSet.h"

#include <type_traits>
#include <unordered_map>

namespace tpa
{

template <typename T>
class PtsMap
{
private:
	static_assert(std::is_pointer<T>::value, "PtsMap only accept pointer as key type");

	using MapType = std::unordered_map<T, PtsSet>;
	MapType mapping;
public:
	using const_iterator = typename MapType::const_iterator;

	PtsSet lookup(T key) const
	{
		assert(key != nullptr);
		auto itr = mapping.find(key);
		if (itr == mapping.end())
			return PtsSet::getEmptySet();
		else
			return itr->second;
	}
	bool contains(T key) const
	{
		return !lookup(key).empty();
	}

	bool insert(T key, const MemoryObject* obj)
	{
		assert(key != nullptr && obj != nullptr);

		auto itr = mapping.find(key);
		if (itr == mapping.end())
			itr = mapping.insert(std::make_pair(key, PtsSet::getEmptySet())).first;

		auto& set = itr->second;
		auto newSet = set.insert(obj);
		if (set == newSet)
			return false;
		else
		{
			set = newSet;
			return true;
		}
	}

	bool weakUpdate(T key, PtsSet pSet)
	{
		assert(key != nullptr);

		auto itr = mapping.find(key);
		if (itr == mapping.end())
		{
			mapping.insert(std::make_pair(key, pSet));
			return true;
		}
		else
		{
			auto& set = itr->second;
			auto newSet = set.merge(pSet);
			if (newSet == set)
				return false;
			else
			{
				set = newSet;
				return true;
			}
		}
	}

	bool strongUpdate(T key, PtsSet pSet)
	{
		assert(key != nullptr);

		auto itr = mapping.find(key);
		if (itr == mapping.end())
		{
			mapping.insert(std::make_pair(key, pSet));
			return true;
		}
		else
		{
			auto& set = itr->second;
			if (set == pSet)
				return false;
			else
			{
				set = pSet;
				return true;
			}
		}
	}

	bool mergeWith(const PtsMap<T>& rhs)
	{
		bool ret = false;
		for (auto const& mapping: rhs)
			ret |= weakUpdate(mapping.first, mapping.second);
		return ret;
	}

	size_t size() const { return mapping.size(); }
	bool empty() const { return mapping.empty(); }
	const_iterator begin() const { return mapping.begin(); }
	const_iterator end() const { return mapping.end(); }
};

}