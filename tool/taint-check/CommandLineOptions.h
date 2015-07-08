#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileName;

	llvm::StringRef ptrConfigFileName;
	llvm::StringRef modRefConfigFileName;
	llvm::StringRef taintConfigFileName;
	bool noPrepassFlag;
	unsigned k;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileName() const { return inputFileName; }

	const llvm::StringRef& getPtrConfigFileName() const { return ptrConfigFileName; }
	const llvm::StringRef& getModRefConfigFileName() const { return modRefConfigFileName; }
	const llvm::StringRef& getTaintConfigFileName() const { return taintConfigFileName; }
	bool isPrepassDisabled() const { return noPrepassFlag; }
	unsigned getContextSensitivity() const { return k; }
};
