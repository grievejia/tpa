#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTablePrinter.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static void printPosition(raw_ostream& os, const APosition& pos)
{
	if (pos.isReturnPosition())
		os << "(return)";
	else
	{
		auto const& argPos = pos.getAsArgPosition();
		os << "(arg " << static_cast<unsigned>(argPos.getArgIndex()) << ")";
	}
}

static void printAllocEffect(raw_ostream& os, const PointerAllocEffect& allocEffect)
{
	os << "  Memory allocation with ";
	if (allocEffect.hasArgPosition())
	{
		os << "size given by ";
		printPosition(os, allocEffect.getSizePosition());
	}
	else
	{
		os << "unknown size";
	}
}

static void printCopySource(raw_ostream& os, const CopySource& src)
{
	switch (src.getType())
	{
		case CopySource::SourceType::ArgValue:
			printPosition(os, src.getPosition());
			break;
		case CopySource::SourceType::ArgDirectMemory:
			os << "*";
			printPosition(os, src.getPosition());
			break;
		case CopySource::SourceType::ArgReachableMemory:
			os << "*[";
			printPosition(os, src.getPosition());
			os << " + x]";
			break;
		case CopySource::SourceType::Null:
			os << "[Null]";
			break;
		case CopySource::SourceType::Universal:
			os << "[Unknown]";
			break;
		case CopySource::SourceType::Static:
			os << "[Static]";
			break;
	}
}

static void printCopyEffect(raw_ostream& os, const PointerCopyEffect& copyEffect)
{
	os << "  Copy points-to set from ";
	printCopySource(os, copyEffect.getSource());
	os << " to ";
	printPosition(os, copyEffect.getDest());
}

static void printPointerEffect(raw_ostream& os, const PointerEffect& effect)
{
	switch (effect.getType())
	{
		case PointerEffectType::Alloc:
			printAllocEffect(os, effect.getAsAllocEffect());
			break;
		case PointerEffectType::Copy:
			printCopyEffect(os, effect.getAsCopyEffect());
			break;
	}
	os << "\n";
}

void ExternalPointerTablePrinter::printTable(const ExternalPointerTable& table)
{
	os << "\n----- ExternalPointerTable -----\n";
	for (auto const& mapping: table)
	{
		os << "Function " << mapping.first << ":\n";
		
		if (mapping.second.isEmpty())
		{
			os << "  Ignored\n";
			continue;
		}

		for (auto const& effect: mapping.second)
			printPointerEffect(os, effect);
	}
	os << "--------- End of Table ---------\n\n";
}

}