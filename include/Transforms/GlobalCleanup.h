#ifndef TPA_GLOBAL_CLEANUP_H
#define TPA_GLOBAL_CLEANUP_H

#include "llvm/Pass.h"

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

#endif