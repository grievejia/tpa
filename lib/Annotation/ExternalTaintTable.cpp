#include "Annotation/Taint/ExternalTaintTable.h"
#include "Util/IO/ReadFile.h"
#include "Util/pcomb/pcomb.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace pcomb;

namespace annotation
{

ExternalTaintTable ExternalTaintTable::buildTable(const llvm::StringRef& fileContent)
{
	ExternalTaintTable table;

	auto idx = rule(
		regex("\\d+"),
		[] (auto const& digits) -> uint8_t
		{
			auto num = std::stoul(digits.data());
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
			return taint::TaintLattice::Tainted;
		}
	);
	auto ulattice = rule(
		ch('U'),
		[] (char)
		{
			return taint::TaintLattice::Untainted;
		}
	);
	auto elattice = rule(
		ch('E'),
		[] (char)
		{
			return taint::TaintLattice::Either;
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
		errs() << "Parsing taint config file failed at line " << stream.getLineNumber() << ", column " << stream.getColumnNumber() << "\n";
		std::exit(-1);
	}

	return table;
}

ExternalTaintTable ExternalTaintTable::loadFromFile(const char* fileName)
{
	auto memBuf = util::io::readFileIntoBuffer(fileName);
	return buildTable(memBuf->getBuffer());
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