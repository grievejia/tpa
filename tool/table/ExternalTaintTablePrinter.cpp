#include "Annotation/Taint/ExternalTaintTable.h"
#include "TablePrinter/ExternalTaintTablePrinter.h"

#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace annotation
{

static void printTLattice(raw_ostream& os, taint::TaintLattice l)
{
	os << ", Value = ";
	switch (l)
	{
		case taint::TaintLattice::Tainted:
			os << "{TAINT}";
			break;
		case taint::TaintLattice::Untainted:
			os << "{UNTAINT}";
			break;
		case taint::TaintLattice::Either:
			os << "{EITHER}";
			break;
		case taint::TaintLattice::Unknown:
			llvm_unreachable("Shouldn't get unknown entry in taint table");
	}
}

static void printTClass(raw_ostream& os, TClass c)
{
	if (c == TClass::ValueOnly)
		os << "(ValueOnly)";
	else
		os << "(DirectMemory)";
}

static void printTPos(raw_ostream& os, TPosition pos)
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

static void printTEntry(raw_ostream& os, const TaintEntry& entry)
{
	switch (entry.getEntryEnd())
	{
		case TEnd::Source:
		{
			auto const& sourceEntry = entry.getAsSourceEntry();
			os.changeColor(raw_ostream::MAGENTA);
			
			os << "  Taint source at ";
			printTPos(os, sourceEntry.getTaintPosition());
			printTClass(os, sourceEntry.getTaintClass());
			printTLattice(os, sourceEntry.getTaintValue());
			break;
		}
		case TEnd::Pipe:
		{
			auto const& pipeEntry = entry.getAsPipeEntry();
			os.changeColor(raw_ostream::BLUE);

			os << "  Taint pipe from ";
			printTPos(os, pipeEntry.getSrcPosition());
			printTClass(os, pipeEntry.getSrcClass());
			os << " to ";
			printTPos(os, pipeEntry.getDstPosition());
			printTClass(os, pipeEntry.getDstClass());
			break;
		}
		case TEnd::Sink:
		{
			auto const& sinkEntry = entry.getAsSinkEntry();
			os.changeColor(raw_ostream::YELLOW);

			os << "  Taint sink at ";
			printTPos(os, sinkEntry.getArgPosition());
			printTClass(os, sinkEntry.getTaintClass());
			break;
		}
	}
	os << "\n";
}

void ExternalTaintTablePrinter::printTable(const ExternalTaintTable& table) const
{
	os << "\n----- ExternalTaintTable -----\n";
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

		for (auto const& entry: mapping.second)
			printTEntry(os, entry);
	}

	os.resetColor();
	os << "---------- End of Table ---------\n";
}

}
