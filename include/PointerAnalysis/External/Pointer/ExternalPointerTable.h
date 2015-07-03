#ifndef TPA_EXTERNAL_POINTER_TABLE_H
#define TPA_EXTERNAL_POINTER_TABLE_H

#include "PointerAnalysis/External/Pointer/PointerEffectSummary.h"

#include <llvm/ADT/StringRef.h>

#include <unordered_map>

namespace tpa
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

	const_iterator begin() const { return table.begin(); }
	const_iterator end() const { return table.end(); }
	size_t size() const { return table.size(); }

	static ExternalPointerTable loadFromFile(const llvm::StringRef& fileName);
	static ExternalPointerTable loadFromFile();
};

}

#endif
