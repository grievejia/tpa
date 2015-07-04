#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Annotation/Pointer/ExternalPointerTablePrinter.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace annotation
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
	if (allocEffect.hasSizePosition())
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
		case CopySource::SourceType::Value:
			printPosition(os, src.getPosition());
			break;
		case CopySource::SourceType::DirectMemory:
			os << "*";
			printPosition(os, src.getPosition());
			break;
		case CopySource::SourceType::ReachableMemory:
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

static void printCopyDest(raw_ostream& os, const CopyDest& dest)
{
	switch (dest.getType())
	{
		case CopyDest::DestType::Value:
			printPosition(os, dest.getPosition());
			break;
		case CopyDest::DestType::DirectMemory:
			os << "*";
			printPosition(os, dest.getPosition());
			break;
		case CopyDest::DestType::ReachableMemory:
			os << "*[";
			printPosition(os, dest.getPosition());
			os << " + x]";
			break;
	}
}

static void printCopyEffect(raw_ostream& os, const PointerCopyEffect& copyEffect)
{
	os << "  Copy points-to set from ";
	printCopySource(os, copyEffect.getSource());
	os << " to ";
	printCopyDest(os, copyEffect.getDest());
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

		os.changeColor(raw_ostream::YELLOW);
		for (auto const& effect: mapping.second)
			printPointerEffect(os, effect);
	}

	os.resetColor();
	os << "--------- End of Table ---------\n\n";
}

}