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
		os << "arg " << static_cast<unsigned>(pos.getArgIndex());
		if (pos.isAllArgPosition())
			os << " and after";
	}
}

void TablePrinter::printTEntry(const TaintEntry& entry) const
{
	if (auto sourceEntry = dyn_cast<SourceTaintEntry>(&entry))
	{
		os << "  Taint source at ";
		printTPos(sourceEntry->getTaintPosition());
	}
	else if (auto pipeEntry = dyn_cast<PipeTaintEntry>(&entry))
	{
		os << "  Taint pipe from ";
		printTPos(pipeEntry->getSrcPosition());
		printTClass(pipeEntry->getSrcClass());
		os << " to ";
		printTPos(pipeEntry->getDstPosition());
		printTClass(pipeEntry->getDstClass());
	}
	else if (auto sinkEntry = dyn_cast<SinkTaintEntry>(&entry))
	{
		os << "  Taint sink at ";
		printTPos(sinkEntry->getArgPosition());
		printTClass(sinkEntry->getTaintClass());
	}
	else
	{
		os << "  Ignored";
	}
	os << "\n";
}

void TablePrinter::printTable(const SourceSinkLookupTable& table) const
{
	os << "\n----- TablePrinter -----\n";
	for (auto const& mapping: table)
	{
		os << "Function " << mapping.first << ":\n";
		for (auto const& entry: mapping.second)
			printTEntry(entry);
	}
	os << "---------- End of Table ---------\n";
}

}
}