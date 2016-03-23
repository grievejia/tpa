#include "TableChecker.h"
#include "Util/IO/ReadIR.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

static cl::opt<std::string> BitcodeFileName(cl::Positional, cl::desc("<bitcode file>"), cl::Required);
static cl::opt<std::string> PtrTableFileName("ptr", cl::desc("Pointer effect annotation filename"));
static cl::opt<std::string> ModRefTableFileName("modref", cl::desc("Function mod/ref annotation filename"));
static cl::opt<std::string> TaintTableFileName("taint", cl::desc("Taintness annotation filename"));

void printVersion()
{
	outs() << "table-check for TPA, version 0.1\n"; 
}

int main(int argc, char** argv)
{
	cl::SetVersionPrinter(printVersion);
	cl::ParseCommandLineOptions(argc, argv, "External function annotation validation tool\n\n  The table-check tool can be used to check if the given annotation files are sufficient to analyze a specific bitcode file\n");

	if (PtrTableFileName.empty() && TaintTableFileName.empty() && ModRefTableFileName.empty())
	{
		outs() << argv[0] << ": Did not specify any table!\n\n";
		outs() << "At least one of the following option must be specified:\n  -ptr\n  -modref\n  -taint\n";
		std::exit(-1);
	}

	auto module = util::io::readModuleFromFile(BitcodeFileName.data());

	if (!PtrTableFileName.empty())
	{
		errs() << "\nChecking pointer effect table...\n";
		checkPtrEffectTable(*module, PtrTableFileName);
	}

	if (!ModRefTableFileName.empty())
	{
		errs() << "\nChecking mod/ref table...\n";
		checkModRefEffectTable(*module, ModRefTableFileName);
	}

	if (!TaintTableFileName.empty())
	{
		errs() << "\nChecking taint effect table...\n";
		checkTaintTable(*module, TaintTableFileName);
	}

	return 0;
}