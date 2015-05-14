#ifndef TPA_EXTERNAL_MOD_REF_TABLE_H
#define TPA_EXTERNAL_MOD_REF_TABLE_H

#include "PointerAnalysis/External/ModRef/ModRefEffectSummary.h"

#include <unordered_map>

namespace llvm
{
	class StringRef;
}

namespace tpa
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
	size_t getSize() const { return table.size(); }

	static ExternalModRefTable loadFromFile(const llvm::StringRef& fileName);
	static ExternalModRefTable loadFromFile();
};

}

#endif
