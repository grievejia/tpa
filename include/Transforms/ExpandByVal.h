#pragma once

#include "llvm/Pass.h"

namespace transform
{

class ExpandByValPass: public llvm::ModulePass
{
public:
	static char ID; // Pass identification, replacement for typeid
	ExpandByValPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module &M) override;
};

}
