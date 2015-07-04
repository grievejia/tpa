#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileName;
	llvm::StringRef outputDirName;

	bool dryRunFlag;
	bool noPrepassFlag;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileName() { return inputFileName; }
	const llvm::StringRef& getOutputDirName() const { return outputDirName; }

	bool isDryRun() const { return dryRunFlag; }
	bool isPrepassDisabled() const { return noPrepassFlag; }
};