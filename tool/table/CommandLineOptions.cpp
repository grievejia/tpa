#include "CommandLineOptions.h"
#include "Util/CommandLine/TypedCommandLineParser.h"

using namespace util;

CommandLineOptions::CommandLineOptions(int argc, char** argv)
{
	TypedCommandLineParser cmdParser("Annotation pretty printer");
	cmdParser.addStringPositionalFlag("inputType", "Type of the input annotation (choices: ptr, modref, taint)", inputFileType);
	cmdParser.addStringPositionalFlag("inputFile", "Input annotation file name", inputFileName);

	cmdParser.parseCommandLineOptions(argc, argv);
}