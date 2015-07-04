#pragma once

#include <llvm/ADT/StringRef.h>

class CommandLineOptions
{
private:
	llvm::StringRef inputFileType;
	llvm::StringRef inputFileName;
public:
	CommandLineOptions(int argc, char** argv);

	const llvm::StringRef& getInputFileType() const { return inputFileType; }
	const llvm::StringRef& getInputFileName() const { return inputFileName; }
};
