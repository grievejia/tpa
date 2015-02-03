#ifndef TPA_EXPAND_BYVAL_H
#define TPA_EXPAND_BYVAL_H

#include "llvm/Pass.h"

class ExpandByValPass: public llvm::ModulePass
{
public:
	static char ID; // Pass identification, replacement for typeid
	ExpandByValPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module &M) override;
};

#endif
