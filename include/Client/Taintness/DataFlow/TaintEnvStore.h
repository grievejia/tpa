#ifndef TPA_TAINT_ENV_STORE_H
#define TPA_TAINT_ENV_STORE_H

#include "Client/Lattice/TaintLattice.h"
#include "MemoryModel/Precision/ProgramLocation.h"

#include <llvm/ADT/DenseMap.h>

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

	TaintLattice lookup(KeyType key) const
	{
		auto itr = taintMap.find(key);
		if (itr != taintMap.end())
			return itr->second;
		else
			return TaintLattice::Unknown;
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

using TaintEnv = TaintMap<std::pair<const tpa::Context*, const llvm::Value*>>;
using TaintStore = TaintMap<const tpa::MemoryLocation*>;

}
}

#endif
