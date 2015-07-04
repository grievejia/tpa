#pragma once

#include "PointerAnalysis/Support/ProgramPoint.h"
#include "PointerAnalysis/Support/Store.h"

#include <unordered_map>

namespace tpa
{

class Memo
{
private:
	using MapType = std::unordered_map<ProgramPoint, Store>;
	MapType inState;
public:
	Memo() = default;

	Memo(const Memo&) = delete;
	Memo(Memo&&) noexcept = default;
	Memo& operator=(const Memo&) = delete;
	Memo& operator=(Memo&&) = delete;

	// Return NULL if store not found
	const Store* lookup(const ProgramPoint& pp) const
	{
		auto itr = inState.find(pp);
		if (itr == inState.end())
			return nullptr;
		else
			return &itr->second;
	}

	// Return true if memo changes
	template <typename StoreType>
	bool update(const ProgramPoint& pp, StoreType&& store)
	{
		static_assert(std::is_same<std::remove_cv_t<std::remove_reference_t<StoreType>>, Store>::value, "Memo.update() only accept Store");
		auto itr = inState.find(pp);
		if (itr == inState.end())
		{
			inState.insert(std::make_pair(pp, std::forward<StoreType>(store)));
			return true;
		}
		else
		{
			return itr->second.mergeWith(store);
		}
	}

	bool update(const ProgramPoint& pp, const MemoryObject* obj, PtsSet pSet)
	{
		auto itr = inState.find(pp);
		if (itr == inState.end())
		{
			auto newStore = Store();
			newStore.strongUpdate(obj, pSet);
			inState.insert(itr, std::make_pair(pp, std::move(newStore)));
			return true;
		}
		else
		{
			return itr->second.weakUpdate(obj, pSet);
		}
	}

	bool empty() const { return inState.empty(); }
	void clear() { inState.clear(); }
};

}
