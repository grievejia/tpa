#include "Client/Taintness/SourceSink/Table/SourceSinkLookupTable.h"
#include "Client/Taintness/SourceSink/Table/TablePrinter.h"

#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace client
{
namespace taint
{

TablePrinter::TablePrinter(raw_ostream& o): os(o)
{
}

void TablePrinter::printTClass(TClass c) const
{
	if (c == TClass::ValueOnly)
		os << "(ValueOnly)";
	else
		os << "(DirectMemory)";
}

void TablePrinter::printTPos(TPosition pos) const
{
	if (pos.isReturnPosition())
		os << "return";
	else
	{
		auto const& argPos = pos.getAsArgPosition();
		os << "arg " << static_cast<unsigned>(argPos.getArgIndex());
		if (argPos.isAfterArgPosition())
			os << " and after";
	}
}

void TablePrinter::printTEntry(const TaintEntry& entry) const
{
	switch (entry.getEntryEnd())
	{
		case TEnd::Source:
		{
			auto const& sourceEntry = entry.getAsSourceEntry();
			
			os << "  Taint source at ";
			printTPos(sourceEntry.getTaintPosition());
			break;
		}
		case TEnd::Pipe:
		{
			auto const& pipeEntry = entry.getAsPipeEntry();

			os << "  Taint pipe from ";
			printTPos(pipeEntry.getSrcPosition());
			printTClass(pipeEntry.getSrcClass());
			os << " to ";
			printTPos(pipeEntry.getDstPosition());
			printTClass(pipeEntry.getDstClass());
			break;
		}
		case TEnd::Sink:
		{
			auto const& sinkEntry = entry.getAsSinkEntry();

			os << "  Taint sink at ";
			printTPos(sinkEntry.getArgPosition());
			printTClass(sinkEntry.getTaintClass());
			break;
		}
	}
	os << "\n";
}

void TablePrinter::printTable(const SourceSinkLookupTable& table) const
{
	os << "\n----- TablePrinter -----\n";
	for (auto const& mapping: table)
	{
		os << "Function " << mapping.first << ":\n";
		
		if (mapping.second.isEmpty())
		{
			os << "  Ignored\n";
			continue;
		}

		for (auto const& entry: mapping.second)
			printTEntry(entry);
	}
	os << "---------- End of Table ---------\n";
}

}
}