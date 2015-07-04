#pragma once

#include <unordered_map>

namespace llvm
{
	class Type;
}

namespace tpa
{

class TypeLayout;

class TypeMap
{
private:
	using MapType = std::unordered_map<const llvm::Type*, const TypeLayout*>;
	MapType typeMap;
public:
	using const_iterator = MapType::const_iterator;

	TypeMap() = default;

	// The insert() function here performs strong update
	void insert(const llvm::Type*, const TypeLayout*);
	const TypeLayout* lookup(const llvm::Type*) const;

	const_iterator begin() const { return typeMap.begin(); }
	const_iterator end() const { return typeMap.end(); }
};

}