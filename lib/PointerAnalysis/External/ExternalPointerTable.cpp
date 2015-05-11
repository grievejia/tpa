#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "Utils/pcomb/pcomb.h"
#include "Utils/ReadFile.h"

using namespace llvm;
using namespace pcomb;

namespace tpa
{

const PointerEffectSummary* ExternalPointerTable::getSummary(const llvm::StringRef& name) const
{
	auto itr = table.find(name);
	if (itr == table.end())
		return nullptr;
	else
		return &itr->second;
}

void ExternalPointerTable::buildTable(ExternalPointerTable& extTable, const StringRef& fileContent)
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

	auto pret = rule(
		token(str("Ret")),
		[] (auto const&)
		{
			return APosition::getReturnPosition();
		}
	);

	auto parg = rule(
		token(seq(str("Arg"), idx)),
		[] (auto const& pair)
		{
			return APosition::getArgPosition(std::get<1>(pair));
		}
	);

	auto ppos = alt(parg, pret);

	auto argsrc = rule(
		seq(parg, token(alt(ch('V'), ch('D'), ch('R')))),
		[] (auto const& pair)
		{
			auto type = std::get<1>(pair);
			switch (type)
			{
				case 'V':
					return CopySource::getArgValue(std::get<0>(pair));
				case 'D':
					return CopySource::getArgDirectMemory(std::get<0>(pair));
				case 'R':
					return CopySource::getArgRechableMemory(std::get<0>(pair));
				default:
					llvm_unreachable("Only VDR could possibly be here");
			}
		}
	);

	auto nullsrc = rule(
		token(str("NULL")),
		[] (auto const&)
		{
			return CopySource::getNullPointer();
		}
	);

	auto unknowsrc = rule(
		token(str("UNKNOWN")),
		[] (auto const&)
		{
			return CopySource::getUniversalPointer();
		}
	);

	auto staticsrc = rule(
		token(str("STATIC")),
		[] (auto const&)
		{
			return CopySource::getStaticPointer();
		}
	);

	auto copysrc = alt(nullsrc, unknowsrc, staticsrc, argsrc);

	auto ignoreEntry = rule(
		seq(
			token(str("IGNORE")),
			id
		),
		[&extTable] (auto const& pair)
		{
			assert(extTable.getSummary(std::get<1>(pair)) == nullptr && "Ignore entry should not co-exist with other entries");
			extTable.table.insert(std::make_pair(std::get<1>(pair), PointerEffectSummary()));
			return false;
		}
	);

	auto allocWithSize = rule(
		seq(
			token(str("ALLOC")),
			parg
		),
		[&extTable] (auto const& pair)
		{
			return PointerEffect::getAllocEffect(std::get<1>(pair));
		}
	);

	auto allocWithoutSize = rule(
		token(str("ALLOC")),
		[] (auto const&)
		{
			return PointerEffect::getAllocEffect();
		}
	);

	auto allocEntry = rule(
		seq(
			id,
			alt(allocWithSize, allocWithoutSize)
		),
		[&extTable] (auto&& pair)
		{
			extTable.table[std::get<0>(pair)].addEffect(std::move(std::get<1>(pair)));
			return true;
		}
	);

	auto copyEntry = rule(
		seq(
			id,
			ppos,
			copysrc
		),
		[&extTable] (auto const& tuple)
		{
			auto entry = PointerEffect::getCopyEffect(std::get<1>(tuple), std::get<2>(tuple));
			extTable.table[std::get<0>(tuple)].addEffect(std::move(entry));
			return true;
		}
	);

	auto pentry = alt(ignoreEntry, allocEntry, copyEntry);
	auto ptable = many(pentry);

	auto parseResult = ptable.parse(fileContent);
	if (parseResult.hasError() || !StringRef(parseResult.getInputStream().getRawBuffer()).ltrim().empty())
	{
		auto& stream = parseResult.getInputStream();
		auto lineStr = std::to_string(stream.getLineNumber());
		auto colStr = std::to_string(stream.getColumnNumber());
		auto errMsg = Twine("Parsing taint config file failed at line ") + lineStr + ", column " + colStr;
		report_fatal_error(errMsg);
	}
}

ExternalPointerTable ExternalPointerTable::loadFromFile(const llvm::StringRef& fileName)
{
	ExternalPointerTable table;

	auto memBuf = readFileIntoBuffer(fileName);
	buildTable(table, memBuf->getBuffer());

	return table;
}

}