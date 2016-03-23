#pragma once

namespace llvm
{
	class Module;
}

class CommandLineOptions;

void runAnalysisOnModule(const llvm::Module&, const CommandLineOptions&);
