#include "Utils/pcomb/pcomb.h"
#include "Utils/ReadFile.h"
#include "Client/Taintness/SourceSink/Table/ExternalTaintTable.h"

#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace pcomb;

cl::opt<std::string> ConfigFileName("taint-config", cl::desc("Specify taint config filename"), cl::init("taint.conf"), cl::value_desc("filename"));

namespace client
{
namespace taint
{

ExternalTaintTable ExternalTaintTable::buildTableFromBuffer(const StringRef& fileContent)
{
	ExternalTaintTable table;

	auto idx = rule(
		regex("\\d+"),
		[] (auto const& digits) -> uint8_t
		{
			auto num = std::stoul(digits);
			assert(num < 256);
			return num;
		}
	);

	auto id = regex("[\\w\\.]+");

	auto tret = rule(
		str("Ret"),
		[] (auto const&)
		{
			return TPosition::getReturnPosition();
		}
	);

	auto targ = rule(
		seq(str("Arg"), idx),
		[] (auto const& pair)
		{
			return TPosition::getArgPosition(std::get<1>(pair));
		}
	);

	auto tafterarg = rule(
		seq(str("AfterArg"), idx),
		[] (auto const& pair)
		{
			return TPosition::getAfterArgPosition(std::get<1>(pair));
		}
	);

	auto vclass = rule(
		ch('V'),
		[] (char)
		{
			return TClass::ValueOnly;
		}
	);
	auto dclass = rule(
		ch('D'),
		[] (char)
		{
			return TClass::DirectMemory;
		}
	);
	auto rclass = rule(
		ch('R'),
		[] (char)
		{
			return TClass::ReachableMemory;
		}
	);

	auto tlattice = rule(
		ch('T'),
		[] (char)
		{
			return TaintLattice::Tainted;
		}
	);
	auto ulattice = rule(
		ch('U'),
		[] (char)
		{
			return TaintLattice::Untainted;
		}
	);
	auto elattice = rule(
		ch('E'),
		[] (char)
		{
			return TaintLattice::Either;
		}
	);
	auto tval = alt(tlattice, ulattice, elattice);

	auto srcEntry = rule(
		seq(
			rule(token(str("SOURCE")), [] (auto const&) { return TEnd::Source; }),
			token(id),
			token(alt(tret, targ, tafterarg)),
			token(alt(vclass, dclass, rclass)),
			token(tval)
		),
		[&table] (auto const& tuple)
		{
			auto entry = TaintEntry::getSourceEntry(std::get<2>(tuple), std::get<3>(tuple), std::get<4>(tuple));
			table.summaryMap[std::get<1>(tuple)].addEntry(std::move(entry));
			return true;
		}
	);

	auto pipeEntry = rule(
		seq(
			rule(token(str("PIPE")), [] (auto const&) { return TEnd::Pipe; }),
			token(id),
			token(alt(tret, targ, tafterarg)),
			token(alt(vclass, dclass, rclass)),
			token(targ),
			token(alt(vclass, dclass, rclass))
		),
		[&table] (auto const& tuple)
		{
			auto entry = TaintEntry::getPipeEntry(std::get<2>(tuple), std::get<3>(tuple), std::get<4>(tuple), std::get<5>(tuple));
			table.summaryMap[std::get<1>(tuple)].addEntry(std::move(entry));
			return true;
		}
	);

	auto sinkEntry = rule(
		seq(
			rule(token(str("SINK")), [] (auto const&) { return TEnd::Sink; }),
			token(id),
			token(alt(targ, tafterarg)),
			token(alt(vclass, dclass))
		),
		[&table] (auto const& tuple)
		{
			auto entry = TaintEntry::getSinkEntry(std::get<2>(tuple), std::get<3>(tuple));
			table.summaryMap[std::get<1>(tuple)].addEntry(std::move(entry));
			return true;
		}
	);

	// Ignoring a function is the same as inserting an mapping into summaryMap without adding any entry for it
	auto ignoreEntry = rule(
		seq(
			token(str("IGNORE")),
			token(id)
		),
		[&table] (auto const& tuple)
		{
			table.summaryMap.insert(std::make_pair(std::get<1>(tuple), TaintSummary()));
			return false;
		}
	);

	auto commentEntry = rule(
		token(regex("#.*\\n")),
		[] (auto const&)
		{
			return false;
		}
	);

	auto tentry = alt(commentEntry, ignoreEntry, srcEntry, pipeEntry, sinkEntry);
	auto tsummary = many(tentry);

	auto parseResult = tsummary.parse(fileContent);
	if (parseResult.hasError() || !StringRef(parseResult.getInputStream().getRawBuffer()).ltrim().empty())
	{
		auto& stream = parseResult.getInputStream();
		auto lineStr = std::to_string(stream.getLineNumber());
		auto colStr = std::to_string(stream.getColumnNumber());
		auto errMsg = Twine("Parsing taint config file failed at line ") + lineStr + ", column " + colStr;
		report_fatal_error(errMsg);
		llvm_unreachable("Fatal error");
	}

	return table;
}

ExternalTaintTable ExternalTaintTable::loadFromFile(const std::string& fileName)
{
	auto memBuf = tpa::readFileIntoBuffer(fileName);
	return buildTableFromBuffer(memBuf->getBuffer());
}

ExternalTaintTable ExternalTaintTable::loadFromFile()
{
	return loadFromFile(ConfigFileName);
}

const TaintSummary* ExternalTaintTable::lookup(const std::string& name) const
{
	auto itr = summaryMap.find(name);
	if (itr == summaryMap.end())
		return nullptr;
	else
		return &itr->second;
}

}
}