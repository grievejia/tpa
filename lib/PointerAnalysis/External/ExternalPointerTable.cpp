#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "Utils/pcomb/pcomb.h"
#include "Utils/ReadFile.h"

#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace pcomb;

static cl::opt<std::string> ConfigFileName("ptr-config", cl::desc("Specify pointer effect config filename"), cl::init("ptr_effect.conf"), cl::value_desc("filename"));

namespace tpa
{

const PointerEffectSummary* ExternalPointerTable::lookup(const StringRef& name) const
{
	auto itr = table.find(name);
	if (itr == table.end())
		return nullptr;
	else
		return &itr->second;
}

ExternalPointerTable ExternalPointerTable::buildTable(const StringRef& fileContent)
{
	ExternalPointerTable extTable;

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

	auto pret = rule(
		str("Ret"),
		[] (auto const&)
		{
			return APosition::getReturnPosition();
		}
	);

	auto parg = rule(
		seq(str("Arg"), idx),
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
					return CopySource::getValue(std::get<0>(pair));
				case 'D':
					return CopySource::getDirectMemory(std::get<0>(pair));
				case 'R':
					return CopySource::getRechableMemory(std::get<0>(pair));
				default:
					llvm_unreachable("Only VDR could possibly be here");
			}
		}
	);

	auto nullsrc = rule(
		str("NULL"),
		[] (auto const&)
		{
			return CopySource::getNullPointer();
		}
	);

	auto unknowsrc = rule(
		str("UNKNOWN"),
		[] (auto const&)
		{
			return CopySource::getUniversalPointer();
		}
	);

	auto staticsrc = rule(
		str("STATIC"),
		[] (auto const&)
		{
			return CopySource::getStaticPointer();
		}
	);

	auto copysrc = alt(nullsrc, unknowsrc, staticsrc, argsrc);

	auto copydest = rule(
		seq(ppos, token(alt(ch('V'), ch('D'), ch('R')))),
		[] (auto const& pair)
		{
			auto type = std::get<1>(pair);
			switch (type)
			{
				case 'V':
					return CopyDest::getValue(std::get<0>(pair));
				case 'D':
					return CopyDest::getDirectMemory(std::get<0>(pair));
				case 'R':
					return CopyDest::getRechableMemory(std::get<0>(pair));
				default:
					llvm_unreachable("Only VDR could possibly be here");
			}
		}
	);

	auto commentEntry = rule(
		token(regex("#.*\\n")),
		[] (auto const&)
		{
			return false;
		}
	);

	auto ignoreEntry = rule(
		seq(
			token(str("IGNORE")),
			token(id)
		),
		[&extTable] (auto const& pair)
		{
			assert(extTable.lookup(std::get<1>(pair)) == nullptr && "Ignore entry should not co-exist with other entries");
			extTable.table.insert(std::make_pair(std::get<1>(pair), PointerEffectSummary()));
			return false;
		}
	);

	auto allocWithSize = rule(
		seq(
			str("ALLOC"),
			token(parg)
		),
		[&extTable] (auto const& pair)
		{
			return PointerEffect::getAllocEffect(std::get<1>(pair));
		}
	);

	auto allocWithoutSize = rule(
		str("ALLOC"),
		[] (auto const&)
		{
			return PointerEffect::getAllocEffect();
		}
	);

	auto allocEntry = rule(
		seq(
			token(id),
			token(alt(allocWithSize, allocWithoutSize))
		),
		[&extTable] (auto&& pair)
		{
			extTable.table[std::get<0>(pair)].addEffect(std::move(std::get<1>(pair)));
			return true;
		}
	);

	auto copyEntry = rule(
		seq(
			token(id),
			token(str("COPY")),
			token(copydest),
			token(copysrc)
		),
		[&extTable] (auto const& tuple)
		{
			auto entry = PointerEffect::getCopyEffect(std::get<2>(tuple), std::get<3>(tuple));
			extTable.table[std::get<0>(tuple)].addEffect(std::move(entry));
			return true;
		}
	);

	auto pentry = alt(commentEntry, ignoreEntry, allocEntry, copyEntry);
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

	return extTable;
}

ExternalPointerTable ExternalPointerTable::loadFromFile(const StringRef& fileName)
{
	auto memBuf = readFileIntoBuffer(fileName);
	return buildTable(memBuf->getBuffer());
}

ExternalPointerTable ExternalPointerTable::loadFromFile()
{
	return loadFromFile(ConfigFileName);
}

}