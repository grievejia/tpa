#ifndef TPA_STORE_H
#define TPA_STORE_H

#include "MemoryModel/Memory/Memory.h"
#include "MemoryModel/PtsSet/PtsSetManager.h"

#include <llvm/ADT/DenseMap.h>

#include <memory>

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

template <class KT, class VT>
class Bindings
{
public:
	using KeyType = KT;
	using ValueType = VT;
private:
	using BindingType = llvm::DenseMap<KeyType, ValueType>;
	std::shared_ptr<BindingType> bindings;

	// Clients cannot directly create a Bindings. All Bindings allocations must go through BindingManager
	explicit Bindings(): bindings(std::make_shared<BindingType>()) {}

	// Assume (key, val) is not in binding, insert the pair
	void insert(KeyType key, ValueType val)
	{
		// If the binding is shared, copy-on-write first
		if (!bindings.unique())
			bindings = std::make_shared<BindingType>(*bindings);

		(*bindings)[key] = val;
	}

	// Assume key is in binding, remove the pair
	void remove(KeyType key)
	{
		// If the binding is shared, copy-on-write first
		if (!bindings.unique())
			bindings = std::make_shared<BindingType>(*bindings);

		bindings->erase(key);
	}
public:
	using const_iterator = typename BindingType::const_iterator;

	// If binding contains mapping from key, return true;
	bool contains(KeyType key) const
	{
		return bindings->count(key);
	}
	// Fetches the current binding of the variable in the store
	// Return NULL if binding not found
	ValueType lookup(KeyType key) const
	{
		auto itr = bindings->find(key);
		if (itr != bindings->end())
			return itr->second;
		else
			return nullptr;
	}

	bool operator==(const Bindings& other) const
	{
		auto subset = [] (const Bindings& lhs, const Bindings& rhs)
		{
			for (auto const& mapping: lhs)
			{
				if (auto rhsValue = rhs.lookup(mapping.first))
				{
					if (rhsValue != mapping.second)
						return false;
				}
				else
					return false;
			}
			return true;
		};

		auto mySize = getSize();
		auto otherSize = other.getSize();

		// Always traverse the smaller set of binding
		if (mySize != otherSize)
			return false;
		else if (mySize < otherSize)
			return subset(*this, other);
		else
			return subset(other, *this);
	}
	bool operator!=(const Bindings& other) const
	{
		return !(*this == other);
	}

	const_iterator begin() const { return bindings->begin(); }
	const_iterator end() const { return bindings->end(); }
	unsigned getSize() const { return bindings->size(); }
	bool isEmpty() const { return bindings->empty(); }

	friend class BindingManager;
	friend struct std::hash<Bindings<KT, VT>>;
};

class Store
{
public:
	using PtsSet = VectorSet<const MemoryLocation*>;
	using MappingType = Bindings<const MemoryLocation*, const PtsSet*>;
private:
	MappingType globalMem, stackMem, heapMem;
public:
	Store(const MappingType& g, const MappingType& s, const MappingType& h): globalMem(g), stackMem(s), heapMem(h) {}

	const PtsSet* lookup(const MemoryLocation* loc) const
	{
		assert(loc != nullptr);
		auto obj = loc->getMemoryObject();
		if (obj->isGlobalObject())
			return globalMem.lookup(loc);
		else if (obj->isStackObject())
			return stackMem.lookup(loc);
		else
			return heapMem.lookup(loc);
	}
	bool contains(const MemoryLocation* loc) const
	{
		return lookup(loc) != nullptr;
	}

	unsigned getSize() const { return globalMem.getSize() + stackMem.getSize() + heapMem.getSize(); }
	bool isEmpty() const { return globalMem.isEmpty() && stackMem.isEmpty() && heapMem.isEmpty(); }

	MappingType& getGlobalStore() { return globalMem; }
	const MappingType& getGlobalStore() const { return globalMem; }
	MappingType& getStackStore() { return stackMem; }
	const MappingType& getStackStore() const { return stackMem; }
	MappingType& getHeapStore() { return heapMem; }
	const MappingType& getHeapStore() const { return heapMem; }

	bool operator==(const Store& other) const
	{
		return globalMem == other.globalMem && stackMem == other.stackMem && heapMem == other.heapMem;
	}
	bool operator!=(const Store& other) const
	{
		return !(*this == other);
	}

	void dump(llvm::raw_ostream&) const;

	friend class StoreManager;
};

}

namespace std
{

	template <typename KT, typename VT> struct hash<tpa::Bindings<KT, VT>>
	{
		size_t operator()(const tpa::Bindings<KT, VT>& k) const
		{
			return hash<std::shared_ptr<typename tpa::Bindings<KT, VT>::BindingType>>()(k.bindings);
		}
	};

	template <> struct hash<tpa::Store>
	{
		size_t operator()(const tpa::Store& s) const
		{
			auto h1 = hash<tpa::Store::MappingType>()(s.getGlobalStore());
			auto h2 = hash<tpa::Store::MappingType>()(s.getStackStore());
			auto h3 = hash<tpa::Store::MappingType>()(s.getHeapStore());
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

}

#endif
