#pragma once

namespace llvm
{
	class Module;
	class StringRef;
}

void runAnalysisOnModule(const llvm::Module&, const llvm::StringRef&, bool);
