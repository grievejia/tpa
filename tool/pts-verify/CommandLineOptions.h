#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileName;
	llvm::StringRef inputLogName;

	llvm::StringRef ptrConfigFileName;
	unsigned k;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileName() { return inputFileName; }
	const llvm::StringRef& getInputLogName() { return inputLogName; }

	const llvm::StringRef& getPtrConfigFileName() { return ptrConfigFileName; }
	unsigned getContextSensitivity() const { return k; }
};