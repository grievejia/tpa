#include "CommandLineOptions.h"
#include "TablePrinter/ExternalModRefTablePrinter.h"
#include "TablePrinter/ExternalPointerTablePrinter.h"
#include "TablePrinter/ExternalTaintTablePrinter.h"

#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Annotation/ModRef/ExternalModRefTable.h"
#include "Annotation/Taint/ExternalTaintTable.h"

#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;

template <typename PrinterType>
void printTable(const StringRef& fileName)
{
	using TableType = typename PrinterType::TableType;
	auto table = TableType::loadFromFile(fileName.data());
	PrinterType(outs()).printTable(table);
}

int main(int argc, char** argv)
{
	// Parse command line options
	auto opts = CommandLineOptions(argc, argv);

	auto typeStr = opts.getInputFileType();
	if (typeStr == "ptr")
		printTable<ExternalPointerTablePrinter>(opts.getInputFileName());
	else if (typeStr == "modref")
		printTable<ExternalModRefTablePrinter>(opts.getInputFileName());
	else if (typeStr == "taint")
		printTable<ExternalTaintTablePrinter>(opts.getInputFileName());
	else
	{
		outs() << "Unknown annotation file type: " << typeStr << "\n";
		std::exit(-1);
	}

	return 0;
}