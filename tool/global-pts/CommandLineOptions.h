#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileName;
	llvm::StringRef outputFileName;

	bool noPrepassFlag;
	bool dumpTypeFlag;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileName() { return inputFileName; }
	const llvm::StringRef& getOutputFileName() const { return outputFileName; }

	bool isPrepassDisabled() const { return noPrepassFlag; }
	bool shouldPrintType() const { return dumpTypeFlag; }
};

