#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): outputFileName("-"), ptrConfigFileName("ptr.config"), noPrepassFlag(false), outputTextFlag(false)
{
	TypedCommandLineParser cmdParser("Program instrumentation for dynamic pointer analysis");
	cmdParser.addStringPositionalFlag("inputFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringOptionalFlag("o", "Output LLVM bitcode file name", outputFileName);
	cmdParser.addStringOptionalFlag("ptr-config", "Annotation file for external library points-to analysis (default = <current dir>/ptr.config)", ptrConfigFileName);
	cmdParser.addBooleanOptionalFlag("no-prepass", "Do no run IR cannonicalization before the analysis", noPrepassFlag);
	cmdParser.addBooleanOptionalFlag("S", "Output IR in text format rather than bitcode format", outputTextFlag);

	cmdParser.parseCommandLineOptions(argc, argv);
}