#ifndef TPA_TAINT_SOURCE_SINK_H
#define TPA_TAINT_SOURCE_SINK_H

#include "Client/Taintness/SourceSink/Table/TaintSummary.h"

#include <string>
#include <unordered_map>

namespace llvm
{
	class StringRef;
}

namespace client
{

namespace taint
{

class ExternalTaintTable
{
private:
	std::unordered_map<std::string, TaintSummary> summaryMap;

	static ExternalTaintTable buildTableFromBuffer(const llvm::StringRef&);
public:
	using const_iterator = typename decltype(summaryMap)::const_iterator;

	ExternalTaintTable() = default;

	const TaintSummary* lookup(const std::string& name) const;

	const_iterator begin() const { return summaryMap.begin(); }
	const_iterator end() const { return summaryMap.end(); }
	size_t size() const { return summaryMap.size(); }

	static ExternalTaintTable loadFromFile(const std::string& fileName);
	static ExternalTaintTable loadFromFile();
};

}	// end of taint
}	// end of client

#endif
