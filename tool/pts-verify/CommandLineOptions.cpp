#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): ptrConfigFileName("ptr.config"), k(0)
{
	TypedCommandLineParser cmdParser("Points-to analysis verifier");
	cmdParser.addStringPositionalFlag("irFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringPositionalFlag("logFile", "Input dynamic log file name", inputLogName);
	cmdParser.addStringOptionalFlag("ptr-config", "Annotation file for external library points-to analysis (default = <current dir>/ptr.config)", ptrConfigFileName);
	cmdParser.addUIntOptionalFlag("k", "The size limit of the stack for k-CFA", k);

	cmdParser.parseCommandLineOptions(argc, argv);
}