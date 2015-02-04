#ifndef TPA_STORE_MANAGER_H
#define TPA_STORE_MANAGER_H

#include "MemoryModel/PtsSet/Store.h"

namespace tpa
{

class BindingManager
{
public:
	using BindingType = Store::MappingType;
private:
	// For creating new ptsset
	PtsSetManager& valueManager;
public:
	BindingManager(PtsSetManager& m): valueManager(m) {}

	PtsSetManager& getPtsSetManager() { return valueManager; }

	// Empty Bindings getter
	BindingType getEmptyBinding() const { return BindingType(); }

	// Insert an elem b to the value-set of key a in binds. Return true if binds gets updated
	bool insertBinding(BindingType& binds, typename BindingType::KeyType a, typename PtsSet::ElemType b)
	{
		auto set = binds.lookup(a);
		if (set == nullptr)
			set = valueManager.getEmptySet();

		auto newSet = valueManager.insert(set, b);
		if (newSet == set)
			return false;
		else
		{
			binds.insert(a, newSet);
			return true;
		}
	}

	// Insert the mapping (a->v) to binds. If there is an existing mapping (a->v') in binds, the new value for key a becomes (v union v'). Return true if binds gets updated.
	bool mergeBinding(BindingType& binds, typename BindingType::KeyType a, const PtsSet* v)
	{
		if (auto set = binds.lookup(a))
		{
			auto newSet = valueManager.mergeSet(set, v);
			if (newSet == set)
				return false;
			else
			{
				binds.insert(a, newSet);
				return true;
			}
		}
		else
		{
			binds.insert(a, v);
			return true;
		}
	}

	// Insert the mapping (a->v) to binds. If there is an existing mapping (a->v') in binds, the new value for key a becomes v. Return true if binds gets updated.
	bool updateBinding(BindingType& binds, typename BindingType::KeyType a, const PtsSet* v)
	{
		if (auto set = binds.lookup(a))
		{
			if (v == set)
				return false;
		}	

		binds.insert(a, v);
		return true;
	}

	// Merge binds2 into binds 1. Return true if binds1 changes
	bool mergeInto(BindingType& binds1, const BindingType& binds2)
	{
		if (binds1.bindings == binds2.bindings)
			return false;

		bool isChanged = false;
		for (auto const& mapping: binds2)
			isChanged |= mergeBinding(binds1, mapping.first, mapping.second);
		return isChanged;
	}
};

class StoreManager
{
private:
	BindingManager globalManager, stackManager, heapManager;

	template <typename OpType>
	bool dispatch(Store& store, const MemoryLocation* loc, OpType op)
	{
		auto obj = loc->getMemoryObject();
		if (obj->isGlobalObject())
			return op(globalManager, store.getGlobalStore());
		else if (obj->isStackObject())
			return op(stackManager, store.getStackStore());
		else
			return op(heapManager, store.getHeapStore());
	}
public:
	StoreManager(PtsSetManager& m): globalManager(m), stackManager(m), heapManager(m) {}

	PtsSetManager& getPtsSetManager() { return globalManager.getPtsSetManager(); }

	Store getEmptyStore() const
	{
		return Store(
			globalManager.getEmptyBinding(), 
			stackManager.getEmptyBinding(),
			heapManager.getEmptyBinding()
		);
	}

	Store dropStackStore(const Store& store) const
	{
		return Store(store.getGlobalStore(), stackManager.getEmptyBinding(), store.getHeapStore());
	}

	bool insert(Store& store, const MemoryLocation* loc, typename PtsSet::ElemType elem)
	{
		return dispatch(store, loc,
			[loc, elem] (auto& manager, auto& bindings)
			{
				return manager.insertBinding(bindings, loc, elem);
			}
		);
	}
	bool weakUpdate(Store& store, const MemoryLocation* loc, const PtsSet* pSet)
	{
		return dispatch(store, loc,
			[loc, pSet] (auto& manager, auto& bindings)
			{
				return manager.mergeBinding(bindings, loc, pSet);
			}
		);
	}
	bool strongUpdate(Store& store, const MemoryLocation* loc, const PtsSet* pSet)
	{
		return dispatch(store, loc,
			[loc, pSet] (auto& manager, auto& bindings)
			{
				return manager.updateBinding(bindings, loc, pSet);
			}
		);
	}
	bool mergeStore(Store& store, const Store& store2)
	{
		bool ret = false;
		ret |= globalManager.mergeInto(store.getGlobalStore(), store2.getGlobalStore());
		ret |= stackManager.mergeInto(store.getHeapStore(), store2.getHeapStore());
		ret |= heapManager.mergeInto(store.getStackStore(), store2.getStackStore());
		return ret;
	}
};

}

#endif
