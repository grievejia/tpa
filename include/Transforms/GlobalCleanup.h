#pragma once

#include "llvm/Pass.h"

namespace transform
{

class GlobalCleanup: public llvm::ModulePass
{
public:
	static char ID;
	GlobalCleanup(): ModulePass(ID) {}

	bool runOnModule(llvm::Module &M) override;
};

class ResolveAliases: public llvm::ModulePass
{
public:
	static char ID;
	ResolveAliases(): ModulePass(ID) {}

	bool runOnModule(llvm::Module &M) override;
};

}
