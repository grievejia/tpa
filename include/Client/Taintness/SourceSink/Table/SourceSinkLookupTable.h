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

class SourceSinkLookupTable
{
private:
	std::unordered_map<std::string, TaintSummary> summaryMap;

	static SourceSinkLookupTable buildTableFromBuffer(const llvm::StringRef&);
public:
	using const_iterator = typename decltype(summaryMap)::const_iterator;

	SourceSinkLookupTable() = default;

	const TaintSummary* lookup(const std::string& name) const;

	const_iterator begin() const { return summaryMap.begin(); }
	const_iterator end() const { return summaryMap.end(); }
	size_t getSize() const { return summaryMap.size(); }

	static SourceSinkLookupTable loadFromFile(const std::string& fileName);
	static SourceSinkLookupTable loadFromFile();
};

}	// end of taint
}	// end of client

#endif
