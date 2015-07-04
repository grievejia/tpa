#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): outputDirName("dot/"), dryRunFlag(false), noPrepassFlag(false)
{
	TypedCommandLineParser cmdParser("Pointer CFG to .dot drawer");
	cmdParser.addStringPositionalFlag("inputFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringOptionalFlag("o", "Output directory name (default: dot/)", outputDirName);
	cmdParser.addBooleanOptionalFlag("no-prepass", "Do no run IR cannonicalization before the analysis", noPrepassFlag);
	cmdParser.addBooleanOptionalFlag("dry-run", "Do no dump .dot file. Just run the front end", dryRunFlag);

	cmdParser.parseCommandLineOptions(argc, argv);
}