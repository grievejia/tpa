#pragma once

#include "Util/DataStructure/VectorMap.h"

#include <llvm/ADT/StringRef.h>

namespace util
{

class CommandLineFlags
{
private:
	using MapType = VectorMap<llvm::StringRef, llvm::StringRef>;

	MapType flagMap;

	// Return true if insertion took place, false if override took place
	bool addFlag(const llvm::StringRef& key, const llvm::StringRef& value)
	{
		return flagMap.insert_or_assign(key, value).second;
	}
public:
	using const_iterator = MapType::const_iterator;

	CommandLineFlags() = default;

	const llvm::StringRef* lookup(const llvm::StringRef& key) const
	{
		auto itr = flagMap.find(key);
		if (itr == flagMap.end())
			return nullptr;
		else
			return &itr->second;
	}

	bool count(const llvm::StringRef& key) const
	{
		return lookup(key) != nullptr;
	}

	const_iterator begin() const { return flagMap.begin(); }
	const_iterator end() const { return flagMap.end(); }

	friend class CommandLineParser;
};

}