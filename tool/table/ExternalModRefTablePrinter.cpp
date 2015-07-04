#include "Annotation/ModRef/ExternalModRefTable.h"
#include "TablePrinter/ExternalModRefTablePrinter.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace annotation
{

static void printClass(raw_ostream& os, ModRefClass c)
{
	switch (c)
	{
		case ModRefClass::DirectMemory:
			os << "[DIRECT MEMORY]";
			break;
		case ModRefClass::ReachableMemory:
			os << "[REACHABLE MEMORY]";
			break;
	}
}

static void printEffect(raw_ostream& os, const ModRefEffect& effect)
{
	auto const& pos = effect.getPosition();
	if (pos.isReturnPosition())
	{
		os << "    return";
	}
	else
	{
		auto const& argPos = pos.getAsArgPosition();
		unsigned idx = argPos.getArgIndex();

		os << "    arg" << idx;
		if (argPos.isAfterArgPosition())
			os << " and after";
	}

	os << ", ";
	printClass(os, effect.getClass());
	os << "\n";
}

static void printModEffects(raw_ostream& os, const ModRefEffectSummary& summary)
{
	os << "  [MOD]\n";
	for (auto const& effect: summary)
		if (effect.getType() == ModRefType::Mod)
			printEffect(os, effect);
}

static void printRefEffects(raw_ostream& os, const ModRefEffectSummary& summary)
{
	os << "  [REF]\n";
	for (auto const& effect: summary)
		if (effect.getType() == ModRefType::Ref)
			printEffect(os, effect);
}

void ExternalModRefTablePrinter::printTable(const ExternalModRefTable& table)
{
	os << "\n----- ExternalModRefTable -----\n";
	for (auto const& mapping: table)
	{
		os.resetColor();

		os << "Function ";
		os.changeColor(raw_ostream::GREEN);
		os << mapping.first << ":\n";
		
		if (mapping.second.empty())
		{
			os.changeColor(raw_ostream::RED);
			os << "  Ignored\n";
			continue;
		}

		os.changeColor(raw_ostream::MAGENTA);
		printModEffects(os, mapping.second);
		os.changeColor(raw_ostream::YELLOW);
		printRefEffects(os, mapping.second);
	}

	os.resetColor();
	os << "--------- End of Table ---------\n\n";
}

}
