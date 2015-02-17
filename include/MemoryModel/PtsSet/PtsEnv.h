#ifndef TPA_PTS_ENV_H
#define TPA_PTS_ENV_H

#include "MemoryModel/Memory/Memory.h"
#include "MemoryModel/Pointer/Pointer.h"
#include "MemoryModel/PtsSet/PtsSetManager.h"

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

class Env
{
private:
	PtsSetManager& pSetManager;

	llvm::DenseMap<const Pointer*, const PtsSet*> env;
public:
	using const_iterator = decltype(env)::const_iterator;

	Env(PtsSetManager& m): pSetManager(m) {}
	Env(Env&& other): pSetManager(other.pSetManager), env(std::move(other.env)) {}
	Env(const Env& other): pSetManager(other.pSetManager), env(other.env) {}
	Env& operator=(Env&& other)
	{
		assert(&pSetManager == &other.pSetManager);
		env = std::move(other.env);
		return *this;
	}

	const PtsSet* lookup(const Pointer* ptr) const
	{
		assert(ptr != nullptr);
		auto itr = env.find(ptr);
		if (itr == env.end())
			return nullptr;
		else
			return itr->second;
	}

	bool insertBinding(const Pointer* ptr, const MemoryLocation* loc)
	{
		assert(ptr != nullptr && loc != nullptr);

		auto itr = env.find(ptr);
		if (itr == env.end())
			itr = env.insert(std::make_pair(ptr, pSetManager.getEmptySet())).first;

		auto& set = itr->second;
		auto newSet = pSetManager.insert(itr->second, loc);
		if (set == newSet)
			return false;
		else
		{
			set = newSet;
			return true;
		}
	}

	bool mergeBinding(const Pointer* ptr, const PtsSet* pSet)
	{
		assert(ptr != nullptr && pSet != nullptr);

		auto itr = env.find(ptr);
		if (itr == env.end())
		{
			env.insert(std::make_pair(ptr, pSet));
			return true;
		}
		else
		{
			auto& set = itr->second;
			auto newSet = pSetManager.mergeSet(set, pSet);
			if (newSet == set)
				return false;
			else
			{
				set = newSet;
				return true;
			}
		}
	}

	bool updateBinding(const Pointer* ptr, const PtsSet* pSet)
	{
		assert(ptr != nullptr && pSet != nullptr);

		auto itr = env.find(ptr);
		if (itr == env.end())
		{
			env.insert(std::make_pair(ptr, pSet));
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

	size_t getSize() const { return env.size(); }
	const_iterator begin() const { return env.begin(); }
	const_iterator end() const { return env.end(); }

	void dump(llvm::raw_ostream&) const; 
};

}

#endif
