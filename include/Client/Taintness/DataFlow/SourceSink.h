#ifndef TPA_TAINT_SOURCE_SINK_H
#define TPA_TAINT_SOURCE_SINK_H

#include "Client/Lattice/TaintLattice.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace client
{

namespace taint
{

// TPosition specify where the position of the taintness is
enum class TPosition: std::uint8_t
{
	Ret,
	Arg0,
	Arg1,
	Arg2,
	Arg3,
	Arg4,
	AfterArg0,
	AfterArg1,
	AllArgs,
};

enum class TClass: bool
{
	ValueOnly,
	DirectMemory,
};

enum class TEnd: bool
{
	Source,
	Sink
};

struct TEntry
{
	TPosition pos;
	TClass what;
	TEnd end;
	TaintLattice val;
};

class TSummary
{
private:
	using DeclList = std::vector<TEntry>;
	DeclList effects;
public:
	using const_iterator = DeclList::const_iterator;

	TSummary() = default;

	void addEntry(const TEntry& decl)
	{
		effects.emplace_back(decl);
	}

	std::size_t getSize() const { return effects.size(); }

	const_iterator begin() const { return effects.begin(); }
	const_iterator end() const { return effects.end(); }
};

class SourceSinkLookupTable
{
private:
	std::unordered_map<std::string, TSummary> summaryMap;
public:
	SourceSinkLookupTable() = default;

	void readSummaryFromFile(const std::string& fileName);
	const TSummary* getSummary(const std::string& name) const;
};

}	// end of taint
}	// end of client

#endif
