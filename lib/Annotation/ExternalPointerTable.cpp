#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Util/IO/ReadFile.h"
#include "Util/pcomb/pcomb.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace pcomb;

namespace annotation
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
					return CopySource::getReachableMemory(std::get<0>(pair));
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
					return CopyDest::getReachableMemory(std::get<0>(pair));
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

	auto exitEntry = rule(
		seq(
			token(id),
			token(str("EXIT"))
		),
		[&extTable] (auto const& tuple)
		{
			auto entry = PointerEffect::getExitEffect();
			extTable.table[std::get<0>(tuple)].addEffect(std::move(entry));
			return true;
		}
	);

	auto pentry = alt(commentEntry, ignoreEntry, allocEntry, copyEntry, exitEntry);
	auto ptable = many(pentry);

	auto parseResult = ptable.parse(fileContent);
	if (parseResult.hasError() || !StringRef(parseResult.getInputStream().getRawBuffer()).ltrim().empty())
	{
		auto& stream = parseResult.getInputStream();
		errs() << "Parsing pointer config file failed at line " << stream.getLineNumber() << ", column " << stream.getColumnNumber() << "\n";
		std::exit(-1);
	}

	return extTable;
}

ExternalPointerTable ExternalPointerTable::loadFromFile(const char* fileName)
{
	auto memBuf = util::io::readFileIntoBuffer(fileName);
	return buildTable(memBuf->getBuffer());
}

}