#pragma once

namespace llvm
{
	class Module;
}

class CommandLineOptions;

// Return true if all test passed
bool runAnalysisOnModule(const llvm::Module&, const CommandLineOptions&);