#pragma once

#include "Annotation/Taint/TaintSummary.h"

#include <llvm/ADT/StringRef.h>

#include <unordered_map>

namespace annotation
{

class ExternalTaintTable
{
private:
	std::unordered_map<std::string, TaintSummary> summaryMap;

	static ExternalTaintTable buildTable(const llvm::StringRef&);
public:
	using const_iterator = typename decltype(summaryMap)::const_iterator;

	ExternalTaintTable() = default;

	const TaintSummary* lookup(const std::string& name) const;

	const_iterator begin() const { return summaryMap.begin(); }
	const_iterator end() const { return summaryMap.end(); }
	size_t size() const { return summaryMap.size(); }

	static ExternalTaintTable loadFromFile(const char* fileName);
};

}	// end of taint
