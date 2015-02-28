#ifndef TPA_STORE_MANAGER_H
#define TPA_STORE_MANAGER_H

#include "MemoryModel/PtsSet/Store.h"

namespace tpa
{

class StoreManager
{
private:
	PtsSetManager& pSetManager;
public:
	StoreManager(PtsSetManager& m): pSetManager(m) {}

	PtsSetManager& getPtsSetManager() { return pSetManager; }

	Store getEmptyStore() const
	{
		return Store();
	}

	bool insert(Store& store, const MemoryLocation* loc, typename PtsSet::ElemType elem)
	{
		auto itr = store.mem.find(loc);
		if (itr == store.mem.end())
		{
			store.mem.insert(std::make_pair(loc, pSetManager.getSingletonSet(elem)));
			return true;
		}
		
		auto newSet = pSetManager.insert(itr->second, elem);
		if (newSet == itr->second)
			return false;
		itr->second = newSet;
		return true;
	}
	bool weakUpdate(Store& store, const MemoryLocation* loc, const PtsSet* pSet)
	{
		auto itr = store.mem.find(loc);
		if (itr == store.mem.end())
		{
			store.mem.insert(std::make_pair(loc, pSet));
			return true;
		}

		auto newSet = pSetManager.mergeSet(itr->second, pSet);
		if (newSet == itr->second)
			return false;
		itr->second = newSet;
		return true;
	}
	bool strongUpdate(Store& store, const MemoryLocation* loc, const PtsSet* pSet)
	{
		auto itr = store.mem.find(loc);
		if (itr == store.mem.end())
		{
			store.mem.insert(std::make_pair(loc, pSet));
			return true;
		}

		if (pSet == itr->second)
			return false;
		itr->second = pSet;
		return true;
	}
	bool mergeStore(Store& store, const Store& store2)
	{
		bool ret = false;
		for (auto const& mapping: store2)
			ret |= weakUpdate(store, mapping.first, mapping.second);
		return ret;
	}
};

}

#endif
