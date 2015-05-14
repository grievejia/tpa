#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "Utils/pcomb/pcomb.h"
#include "Utils/ReadFile.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace pcomb;

static cl::opt<std::string> ConfigFileName("modref-config", cl::desc("Specify mod/ref config filename"), cl::init("mod_ref.conf"), cl::value_desc("filename"));

namespace tpa
{

const ModRefEffectSummary* ExternalModRefTable::lookup(const StringRef& name) const
{
	auto itr = table.find(name);
	if (itr == table.end())
		return nullptr;
	else
		return &itr->second;
}

ExternalModRefTable ExternalModRefTable::buildTable(const StringRef& fileContent)
{
	ExternalModRefTable table;

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

	auto marg = rule(
		token(seq(str("Arg"), idx)),
		[] (auto const& pair)
		{
			return APosition::getArgPosition(std::get<1>(pair));
		}
	);

	auto mafterarg = rule(
		token(seq(str("AfterArg"), idx)),
		[] (auto const& pair)
		{
			return APosition::getAfterArgPosition(std::get<1>(pair));
		}	
	);

	auto mret = rule(
		token(str("Ret")),
		[] (auto const&)
		{
			return APosition::getReturnPosition();
		}
	);

	auto mpos = alt(mret, marg, mafterarg);

	auto modtype = rule(
		str("MOD"),
		[] (auto const&)
		{
			return ModRefType::Mod;
		}
	);

	auto reftype = rule(
		str("REF"),
		[] (auto const&)
		{
			return ModRefType::Ref;
		}
	);

	auto mtype = token(alt(modtype, reftype));

	auto mclass = rule(
		token(alt(ch('D'), ch('R'))),
		[] (char c)
		{
			switch (c)
			{
				case 'D':
					return ModRefClass::DirectMemory;		
				case 'R':
					return ModRefClass::ReachableMemory;
				default:
					llvm_unreachable("ModRef type can only be D or R");
			}
			
		}
	);

	auto regularEntry = rule(
		seq(
			id,
			mtype,
			mpos,
			mclass
		),
		[&table] (auto const& tuple)
		{
			auto entry = ModRefEffect(std::get<1>(tuple), std::get<3>(tuple), std::get<2>(tuple));
			table.table[std::get<0>(tuple)].addEffect(std::move(entry));
			return true;
		}
	);

	auto ignoreEntry = rule(
		seq(
			token(str("IGNORE")),
			id
		),
		[&table] (auto const& pair)
		{
			assert(table.lookup(std::get<1>(pair)) == nullptr && "Ignore entry should not co-exist with other entries");
			table.table.insert(std::make_pair(std::get<1>(pair), ModRefEffectSummary()));
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

	auto entry = alt(commentEntry, ignoreEntry, regularEntry);
	auto ptable = many(entry);

	auto parseResult = ptable.parse(fileContent);
	if (parseResult.hasError() || !StringRef(parseResult.getInputStream().getRawBuffer()).ltrim().empty())
	{
		auto& stream = parseResult.getInputStream();
		auto lineStr = std::to_string(stream.getLineNumber());
		auto colStr = std::to_string(stream.getColumnNumber());
		auto errMsg = Twine("Parsing taint config file failed at line ") + lineStr + ", column " + colStr;
		report_fatal_error(errMsg);
	}

	return table;
}

ExternalModRefTable ExternalModRefTable::loadFromFile(const StringRef& fileName)
{
	auto memBuf = readFileIntoBuffer(fileName);
	return buildTable(memBuf->getBuffer());
}

ExternalModRefTable ExternalModRefTable::loadFromFile()
{
	return loadFromFile(ConfigFileName);
}

}
