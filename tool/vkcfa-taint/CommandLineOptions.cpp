#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv): ptrConfigFileName("ptr.config"), modRefConfigFileName("modref.config"), taintConfigFileName("taint.config")
{
	TypedCommandLineParser cmdParser("Points-to analysis verifier");
	cmdParser.addStringPositionalFlag("irFile", "Input LLVM bitcode file name", inputFileName);
	cmdParser.addStringOptionalFlag("ptr-config", "Annotation file for external library points-to analysis (default = <current dir>/ptr.config)", ptrConfigFileName);
	cmdParser.addStringOptionalFlag("modref-config", "Annotation file for external library mod/ref analysis (default = <current dir>/modref.config)", modRefConfigFileName);
	cmdParser.addStringOptionalFlag("taint-config", "Annotation file for external library taint analysis (default = <current dir>/taint.config)", taintConfigFileName);
	cmdParser.addBooleanOptionalFlag("no-prepass", "Do no run IR cannonicalization before the analysis", noPrepassFlag);

	cmdParser.parseCommandLineOptions(argc, argv);
}