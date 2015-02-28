#ifndef TPA_TAINT_ENV_STORE_H
#define TPA_TAINT_ENV_STORE_H

#include "Client/Lattice/TaintLattice.h"
#include "MemoryModel/Precision/ProgramLocation.h"

#include <llvm/ADT/DenseMap.h>

#include <experimental/optional>

namespace llvm
{
	class Value;
}

namespace tpa
{
	class MemoryLocation;
}

namespace client
{
namespace taint
{

template <typename KeyType>
class TaintMap
{
private:
	llvm::DenseMap<KeyType, TaintLattice> taintMap;
public:
	using const_iterator = typename decltype(taintMap)::const_iterator;

	TaintMap() = default;

	std::experimental::optional<TaintLattice> lookup(KeyType key) const
	{
		std::experimental::optional<TaintLattice> ret;
		auto itr = taintMap.find(key);
		if (itr != taintMap.end())
			ret = itr->second;
		
		return ret;
	}

	bool weakUpdate(KeyType key, TaintLattice l)
	{
		auto itr = taintMap.find(key);
		if (itr == taintMap.end())
		{
			taintMap.insert(std::make_pair(key, l));
			return true;
		}
		
		auto newVal = Lattice<TaintLattice>::merge(itr->second, l);
		if (itr->second == newVal)
			return false;
		itr->second = newVal;
		return true;
	}

	bool strongUpdate(KeyType key, TaintLattice l)
	{
		auto itr = taintMap.find(key);
		if (itr == taintMap.end())
		{
			taintMap.insert(std::make_pair(key, l));
			return true;
		}
		
		if (itr->second == l)
			return false;
		itr->second = l;
		return true;
	}

	bool mergeWith(const TaintMap<KeyType>& other)
	{
		auto ret = false;
		for (auto const& mapping: other)
			ret |= weakUpdate(mapping.first, mapping.second);
		return ret;
	}

	const_iterator begin() const { return taintMap.begin(); }
	const_iterator end() const { return taintMap.end(); }
	size_t getSize() const { return taintMap.size(); }
};

using TaintEnv = TaintMap<const llvm::Value*>;
using TaintStore = TaintMap<const tpa::MemoryLocation*>;

class TaintState
{
private:
	TaintEnv env;
	TaintStore store;
public:
	TaintState() = default;
	TaintState(const TaintEnv& e, TaintStore& s): env(e), store(s) {}

	const TaintEnv& getEnv() const { return env; }
	const TaintStore& getStore() const { return store; }
	TaintEnv& getEnv() { return env; }
	TaintStore& getStore() { return store; }

	std::experimental::optional<TaintLattice> lookup(const llvm::Value* key) const
	{
		return env.lookup(key);
	}

	std::experimental::optional<TaintLattice> lookup(const tpa::MemoryLocation* key) const
	{
		return store.lookup(key);
	}

	bool weakUpdate(const llvm::Value* key, TaintLattice l)
	{
		return env.weakUpdate(key, l);
	}

	bool weakUpdate(const tpa::MemoryLocation* key, TaintLattice l)
	{
		return store.weakUpdate(key, l);
	}

	bool strongUpdate(const llvm::Value* key, TaintLattice l)
	{
		return env.strongUpdate(key, l);
	}

	bool strongUpdate(const tpa::MemoryLocation* key, TaintLattice l)
	{
		return store.strongUpdate(key, l);
	}

	bool mergeWith(const TaintState& other)
	{
		auto rEnv = env.mergeWith(other.env);
		auto rStore = store.mergeWith(other.store);
		return rEnv || rStore;
	}
};

}
}

#endif
