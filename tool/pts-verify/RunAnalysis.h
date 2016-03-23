#pragma once

namespace llvm
{
	class Module;
}

// Return true if all test passed
bool runAnalysisOnModule(const llvm::Module&, const char*, const char*, unsigned);