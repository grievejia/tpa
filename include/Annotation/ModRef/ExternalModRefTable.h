#pragma once

#include "Annotation/ModRef/ModRefEffectSummary.h"

#include <unordered_map>

namespace llvm
{
	class StringRef;
}

namespace annotation
{

class ExternalModRefTable
{
private:
	using MapType = std::unordered_map<std::string, ModRefEffectSummary>;
	MapType table;

	static ExternalModRefTable buildTable(const llvm::StringRef&);
public:
	using const_iterator = MapType::const_iterator;

	ExternalModRefTable() = default;

	const ModRefEffectSummary* lookup(const llvm::StringRef&) const;

	const_iterator begin() const { return table.begin(); }
	const_iterator end() const { return table.end(); }
	size_t size() const { return table.size(); }

	static ExternalModRefTable loadFromFile(const char* fileName);
};

}
