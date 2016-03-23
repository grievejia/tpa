#pragma once

#include "Annotation/Pointer/PointerEffectSummary.h"

#include <unordered_map>

namespace llvm
{
	class StringRef;
}

namespace annotation
{

class ExternalPointerTable
{
private:
	using MapType = std::unordered_map<std::string, PointerEffectSummary>;
	MapType table;

	static ExternalPointerTable buildTable(const llvm::StringRef&);
public:
	using const_iterator = MapType::const_iterator;

	const PointerEffectSummary* lookup(const llvm::StringRef& name) const;

	// Note: this function should be used for testing only. The only sensible way of constructing an external table is calling loadFromFile()
	void addEffect(const llvm::StringRef& name, PointerEffect&& e);

	const_iterator begin() const { return table.begin(); }
	const_iterator end() const { return table.end(); }
	size_t size() const { return table.size(); }

	static ExternalPointerTable loadFromFile(const char* fileName);
};

}
