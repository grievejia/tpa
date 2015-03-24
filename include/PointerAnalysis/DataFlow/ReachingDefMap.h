#ifndef TPA_REACHING_DEF_MAP_H
#define TPA_REACHING_DEF_MAP_H

#include "Utils/VectorSet.h"

#include <unordered_map>

namespace tpa
{

class MemoryLocation;

template <typename NodeType>
class ReachingDefStore
{
private:
	using NodeSet = VectorSet<const NodeType*>;
	std::unordered_map<const MemoryLocation*, NodeSet> store;
public:
	using const_iterator = typename decltype(store)::const_iterator;

	bool updateBinding(const MemoryLocation* loc, const NodeType* node)
	{
		auto itr = store.find(loc);
		if (itr == store.end())
			itr = store.insert(itr, std::make_pair(loc, NodeSet()));

		auto& set = itr->second;
		if (set.getSize() == 1 && *set.begin() == node)
			return false;
		set.clear();
		return set.insert(node);
	}
	bool insertBinding(const MemoryLocation* loc, const NodeType* node)
	{
		auto itr = store.find(loc);
		if (itr == store.end())
			itr = store.insert(itr, std::make_pair(loc, NodeSet()));

		return itr->second.insert(node);
	}
	bool mergeWith(const ReachingDefStore& rhs)
	{
		bool changed = false;
		for (auto const& mapping: rhs)
		{
			auto loc = mapping.first;
			auto itr = store.find(loc);
			if (itr == store.end())
			{
				changed = true;
				store.insert(itr, mapping);
			}
			else
			{
				changed |= itr->second.mergeWith(mapping.second);
			}
		}
		return changed;
	}

	// Return NULL if binding does not exist
	const NodeSet* getReachingDefs(const MemoryLocation* loc) const
	{
		auto itr = store.find(loc);
		if (itr == store.end())
			return nullptr;
		else
			return &itr->second;
	}

	const_iterator begin() const { return store.begin(); }
	const_iterator end() const { return store.end(); }
};

template <typename NodeType>
class ReachingDefMap
{
private:
	using StoreType = ReachingDefStore<NodeType>;
	std::unordered_map<const NodeType*, StoreType> rdStore;
public:
	bool update(const NodeType* node, const StoreType& storeUpdate)
	{
		auto itr = rdStore.find(node);
		if (itr == rdStore.end())
		{
			rdStore.insert(std::make_pair(node, storeUpdate));
			return true;
		}
		else
			return itr->second.mergeWith(storeUpdate);
	}
	StoreType& getReachingDefStore(const NodeType* node)
	{
		return rdStore[node];
	}
	const StoreType& getReachingDefStore(const NodeType* node) const
	{
		auto itr = rdStore.find(node);
		assert(itr != rdStore.end());
		return itr->second;
	}
};

}

#endif
