#include "Client/Taintness/DataFlow/SourceSink.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Regex.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace client
{
namespace taint
{

namespace
{

std::unique_ptr<MemoryBuffer> readFileIntoBuffer(const std::string& fileName)
{
	auto fileOrErr = MemoryBuffer::getFile(fileName);
	if (auto ec = fileOrErr.getError())
	{
		auto errMsg = (Twine("Can't open file \'") + fileName + "\' :" + ec.message()).str();
		report_fatal_error(errMsg);
	}

	return std::move(fileOrErr.get());
}

TEntry wordsToTEntry(const SmallVectorImpl<StringRef>& words)
{
	static auto const posNameMap = std::unordered_map<std::string, TPosition> {
		{ "Ret", TPosition::Ret},
		{ "Arg0", TPosition::Arg0},
		{ "Arg1", TPosition::Arg1},
		{ "Arg2", TPosition::Arg2},
		{ "Arg3", TPosition::Arg3},
		{ "Arg4", TPosition::Arg4},
		{ "AfterArg0", TPosition::AfterArg0},
		{ "AfterArg1", TPosition::AfterArg1},
		{ "AllArgs", TPosition::AllArgs},
	};

	if (words.size() != 6)
		report_fatal_error("SourceSink config file format error: missing column");
	if (words[1] != ":")
		report_fatal_error("SourceSink config file format error: missing colon");

	auto tEnd = TEnd::Sink;
	if (words[2] == "src")
		tEnd = TEnd::Source;
	else if (words[2] != "sink")
		report_fatal_error("SourceSink config file format error: source or sink?");

	auto itr = posNameMap.find(words[3]);
	if (itr == posNameMap.end())
		report_fatal_error("SourceSink config file format error: pos?");
	auto tPos = itr->second;

	auto tClass = TClass::ValueOnly;
	if (words[4] == "D")
		tClass = TClass::DirectMemory;
	else if (words[4] != "V")
		report_fatal_error("SourceSink config file format error: class?");

	auto tVal = TaintLattice::Untainted;
	if (words[5] == "TAINT")
		tVal = TaintLattice::Tainted;
	else if (words[5] != "UNTAINT")
		report_fatal_error("SourceSink config file format error: lattice val?");

	return TEntry{ tPos, tClass, tEnd, tVal };
}

}	// end of anonymous namespace

void SourceSinkLookupTable::readSummaryFromFile(const std::string& fileName)
{
	auto memBuf = readFileIntoBuffer(fileName);

	auto lines = SmallVector<StringRef, 16>();
	SplitString(memBuf->getBuffer(), lines, "\n\r");

	auto words = SmallVector<StringRef, 16>();
	for (auto& line: lines)
	{
		if (line.empty() || line.startswith("#"))
			continue;

		line.split(words, " ", -1, false);
		auto entry = wordsToTEntry(words);

		summaryMap[words[0]].addEntry(entry);

		words.clear();
	}
}

const TSummary* SourceSinkLookupTable::getSummary(const std::string& name) const
{
	auto itr = summaryMap.find(name);
	if (itr == summaryMap.end())
		return nullptr;
	else
		return &(itr->second);
}

}	// end of taint
}	// end of client
