#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): outputFileName(""), noPrepassFlag(false), dumpTypeFlag(false)
{
	TypedCommandLineParser cmdParser("Global pointer analysis for LLVM IR");
	cmdParser.addStringPositionalFlag("inputFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringOptionalFlag("o", "Output LLVM bitcode file name", outputFileName);
	cmdParser.addBooleanOptionalFlag("no-prepass", "Do no run IR cannonicalization before the analysis", noPrepassFlag);
	cmdParser.addBooleanOptionalFlag("print-type", "Dump the internal type of the translated values", dumpTypeFlag);

	cmdParser.parseCommandLineOptions(argc, argv);
}