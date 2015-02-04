#ifndef TPA_MEMO_H
#define TPA_MEMO_H

#include "MemoryModel/PtsSet/StoreManager.h"

#include <llvm/ADT/DenseMap.h>

#include <experimental/optional>

namespace tpa
{

class Context;
class PointerCFGNode;

class Memo
{
private:
	using KeyType = std::pair<const Context*, const PointerCFGNode*>;
	using MapType = llvm::DenseMap<KeyType, Store>;

	StoreManager& storeManager;

	MapType memo;
public:
	Memo(StoreManager& s): storeManager(s) {}

	bool hasMemoState(const Context* ctx, const PointerCFGNode* node) const
	{
		return memo.count(std::make_pair(ctx, node));
	}

	std::experimental::optional<Store> lookup(const Context* ctx, const PointerCFGNode* node) const
	{
		auto itr = memo.find(std::make_pair(ctx, node));
		if (itr == memo.end())
			return std::experimental::optional<Store>();
		else
			return itr->second;
	}

	// Return true if the memo changes
	bool updateMemo(const Context* ctx, const PointerCFGNode* node, const Store& store)
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
			return storeManager.mergeStore(itr->second, store);
		}
	}

	void clear() { memo.clear(); }
};

}

#endif
