#include "table.h"
#include "TableManager/TableManager.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;
using namespace table;

static cl::opt<CommandType> Command(
	cl::desc("Choose command type"),
	cl::values(
		clEnumValN(CommandType::List, "list", "list table contents (default)"),
		clEnumValN(CommandType::Validate, "validate", "check the validity of the tables"),
		clEnumValEnd
	),
	cl::init(CommandType::List)
);

static cl::opt<TableType> TableOpt(
	cl::desc("Choose table type"),
	cl::values(
		clEnumValN(TableType::PointerEffect, "ptr", "External pointer effect table (default)"),
		clEnumValN(TableType::ModRef, "modref", "External mod/ref effect table"),
		clEnumValN(TableType::Taint, "taint", "External taint source/sink table"),
		clEnumValEnd
	),
	cl::init(TableType::PointerEffect)
);

static cl::opt<std::string> FileName(cl::Positional, cl::desc("<input file>"), cl::Required);

void printVersion()
{
	outs() << "table for TPA, version 0.1\n"; 
}

void dispatchCommand()
{
	switch (TableOpt)
	{
		case TableType::PointerEffect:
			PointerTableManager(FileName).processCommand(Command);
			break;
		case TableType::ModRef:
			ModRefTableManager(FileName).processCommand(Command);
			break;
		case TableType::Taint:
			TaintTableManager(FileName).processCommand(Command);
			break;
	}
}

int main(int argc, char** argv)
{
	cl::SetVersionPrinter(printVersion);
	cl::ParseCommandLineOptions(argc, argv, "External function annotation management tool\n\n  The table tool can be used to view and edit all kinds of annotations for TPA use\n");

	dispatchCommand();

	return 0;
}