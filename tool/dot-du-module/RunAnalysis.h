#pragma once

namespace llvm
{
	class Module;
	class StringRef;
}

class CommandLineOptions;

void runAnalysisOnModule(const llvm::Module&, const CommandLineOptions&);
