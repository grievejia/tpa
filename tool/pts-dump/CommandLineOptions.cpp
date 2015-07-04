#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): ptrConfigFileName("ptr.config"), noPrepassFlag(false), k(0)
{
	TypedCommandLineParser cmdParser("Points-to set dumper");
	cmdParser.addStringPositionalFlag("inputFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringOptionalFlag("ptr-config", "Annotation file for external library points-to analysis (default = <current dir>/ptr.config)", ptrConfigFileName);
	cmdParser.addUIntOptionalFlag("k", "The size limit of the stack for k-CFA", k);
	cmdParser.addBooleanOptionalFlag("no-prepass", "Do no run IR cannonicalization before the analysis", noPrepassFlag);

	cmdParser.parseCommandLineOptions(argc, argv);
}