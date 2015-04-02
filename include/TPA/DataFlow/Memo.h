#ifndef TPA_MEMO_H
#define TPA_MEMO_H

#include "MemoryModel/PtsSet/Store.h"

#include <llvm/ADT/DenseMap.h>

#include <experimental/optional>

namespace tpa
{

class Context;

template <typename NodeType>
class Memo
{
private:
	using KeyType = std::pair<const Context*, const NodeType*>;
	using MapType = llvm::DenseMap<KeyType, Store>;

	MapType memo;
public:
	Memo() {}

	bool hasMemoState(const Context* ctx, const NodeType* node) const
	{
		return memo.count(std::make_pair(ctx, node));
	}

	std::experimental::optional<Store> lookup(const Context* ctx, const NodeType* node) const
	{
		std::experimental::optional<Store> ret;
		auto itr = memo.find(std::make_pair(ctx, node));
		if (itr != memo.end())
			ret = itr->second;
		return ret;
	}

	// Return true if the memo changes
	bool updateMemo(const Context* ctx, const NodeType* node, const Store& store)
	{
		auto key = std::make_pair(ctx, node);
		auto itr = memo.find(key);
		if (itr == memo.end())
		{
			memo.insert(std::make_pair(key, store));
			return true;
		}
		else
		{
			return itr->second.mergeWith(store);
		}
	}

	bool updateMemo(const Context* ctx, const NodeType* node, const MemoryLocation* loc, PtsSet pSet)
	{
		auto key = std::make_pair(ctx, node);
		auto itr = memo.find(key);
		if (itr == memo.end())
		{
			auto newStore = Store();
			newStore.strongUpdate(loc, pSet);
			memo.insert(std::make_pair(key, std::move(newStore)));
			return true;
		}
		else
		{
			return itr->second.weakUpdate(loc, pSet);
		}
	}

	void clear() { memo.clear(); }
};

}

#endif
