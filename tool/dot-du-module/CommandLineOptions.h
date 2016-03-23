#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileName;
	llvm::StringRef outputDirName;

	llvm::StringRef ptrConfigFileName;
	llvm::StringRef modRefConfigFileName;

	bool dryRunFlag;
	bool noPrepassFlag;
	unsigned k;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileName() { return inputFileName; }
	const llvm::StringRef& getOutputDirName() const { return outputDirName; }

	const llvm::StringRef& getPtrConfigFileName() const { return ptrConfigFileName; }
	const llvm::StringRef& getModRefConfigFileName() const { return modRefConfigFileName; }

	bool isDryRun() const { return dryRunFlag; }
	bool isPrepassDisabled() const { return noPrepassFlag; }
	unsigned getContextSensitivity() const { return k; }
};