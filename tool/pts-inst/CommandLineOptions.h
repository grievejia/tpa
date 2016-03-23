#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileName;
	llvm::StringRef outputFileName;
	llvm::StringRef ptrConfigFileName;

	bool noPrepassFlag;
	bool outputTextFlag;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileName() { return inputFileName; }
	const llvm::StringRef& getOutputFileName() const { return outputFileName; }
	const llvm::StringRef& getPtrConfigFileName() { return ptrConfigFileName; }

	bool isPrepassDisabled() const { return noPrepassFlag; }
	bool isTextOutput() const { return outputTextFlag || outputFileName == "-"; }
};