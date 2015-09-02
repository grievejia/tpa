#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): outputDirName("dot/"), ptrConfigFileName("ptr.config"), modRefConfigFileName("modref.config"), dryRunFlag(false), noPrepassFlag(false), k(0)
{
	TypedCommandLineParser cmdParser("Pointer CFG to .dot drawer");
	cmdParser.addStringPositionalFlag("inputFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringOptionalFlag("o", "Output directory name (default: dot/)", outputDirName);
	cmdParser.addStringOptionalFlag("ptr-config", "Annotation file for external library points-to analysis (default = <current dir>/ptr.config)", ptrConfigFileName);
	cmdParser.addStringOptionalFlag("modref-config", "Annotation file for external library mod/ref analysis (default = <current dir>/modref.config)", modRefConfigFileName);
	cmdParser.addBooleanOptionalFlag("no-prepass", "Do no run IR cannonicalization before the analysis", noPrepassFlag);
	cmdParser.addBooleanOptionalFlag("dry-run", "Do no dump .dot file. Just run the front end", dryRunFlag);
	cmdParser.addUIntOptionalFlag("k", "Context sensitivity of the underlying pointer analysis", k);

	cmdParser.parseCommandLineOptions(argc, argv);
}