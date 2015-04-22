#include "Utils/pcomb/pcomb.h"
#include "Client/Taintness/SourceSink/Table/SourceSinkLookupTable.h"

#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;
using namespace pcomb;

namespace client
{
namespace taint
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

void SourceSinkLookupTable::parseLines(const StringRef& fileContent)
{
	auto idx = rule(
		regex("\\d+"),
		[] (auto const& digits) -> uint8_t
		{
			auto num = std::stoul(digits);
			assert(num < 256);
			return num;
		}
	);

	auto id = token(regex("[\\w\\.]+"));

	auto tret = rule(
		token(str("Ret")),
		[] (auto const&)
		{
			return TPosition::getReturnPosition();
		}
	);

	auto targ = rule(
		token(seq(str("Arg"), idx)),
		[] (auto const& pair)
		{
			return TPosition::getArgPosition(std::get<1>(pair));
		}
	);

	auto tafterarg = rule(
		token(seq(str("AfterArg"), idx)),
		[] (auto const& pair)
		{
			return TPosition::getAfterArgPosition(std::get<1>(pair));
		}
	);

	auto vclass = rule(
		token(ch('V')),
		[] (char)
		{
			return TClass::ValueOnly;
		}
	);
	auto dclass = rule(
		token(ch('D')),
		[] (char)
		{
			return TClass::DirectMemory;
		}
	);
	auto rclass = rule(
		token(ch('R')),
		[] (char)
		{
			return TClass::ReachableMemory;
		}
	);

	auto srcEntry = rule(
		seq(
			rule(token(str("SOURCE")), [] (auto const&) { return TEnd::Source; }),
			id,
			alt(tret, targ, tafterarg)
		),
		[this] (auto const& tuple)
		{
			auto entry = std::make_unique<SourceTaintEntry>(std::get<2>(tuple));
			summaryMap[std::get<1>(tuple)].addEntry(std::move(entry));
			return true;
		}
	);

	auto pipeEntry = rule(
		seq(
			rule(token(str("PIPE")), [] (auto const&) { return TEnd::Pipe; }),
			id,
			alt(tret, targ),
			alt(vclass, dclass, rclass),
			targ,
			alt(vclass, dclass, rclass)
		),
		[this] (auto const& tuple)
		{
			auto entry = std::make_unique<PipeTaintEntry>(std::get<2>(tuple), std::get<3>(tuple), std::get<4>(tuple), std::get<5>(tuple));
			summaryMap[std::get<1>(tuple)].addEntry(std::move(entry));
			return true;
		}
	);

	auto sinkEntry = rule(
		seq(
			rule(token(str("SINK")), [] (auto const&) { return TEnd::Sink; }),
			id,
			alt(targ, tafterarg),
			alt(vclass, dclass)
		),
		[this] (auto const& tuple)
		{
			auto entry = std::make_unique<SinkTaintEntry>(std::get<2>(tuple), std::get<3>(tuple));
			summaryMap[std::get<1>(tuple)].addEntry(std::move(entry));
			return true;
		}
	);

	// Ignoring a function is the same as inserting an mapping into summaryMap without adding any entry for it
	auto ignoreEntry = rule(
		seq(
			token(str("IGNORE")),
			id
		),
		[this] (auto const& tuple)
		{
			summaryMap.insert(std::make_pair(std::get<1>(tuple), TaintSummary()));
			return false;
		}
	);

	auto tentry = alt(srcEntry, pipeEntry, sinkEntry, ignoreEntry);
	auto tsummary = many(tentry, true);

	auto parseResult = tsummary.parse(fileContent);
	if (parseResult.hasError() || !StringRef(parseResult.getInputStream().getRawBuffer()).ltrim().empty())
	{
		auto& stream = parseResult.getInputStream();
		auto lineStr = std::to_string(stream.getLineNumber());
		auto colStr = std::to_string(stream.getColumnNumber());
		auto errMsg = Twine("Parsing taint config file failed at line ") + lineStr + ", column " + colStr;
		report_fatal_error(errMsg);
	}
}

void SourceSinkLookupTable::readSummaryFromFile(const std::string& fileName)
{
	auto memBuf = readFileIntoBuffer(fileName);
	parseLines(memBuf->getBuffer());
}

const TaintSummary* SourceSinkLookupTable::getSummary(const std::string& name) const
{
	auto itr = summaryMap.find(name);
	if (itr == summaryMap.end())
		return nullptr;
	else
		return &itr->second;
}

}
}