#ifndef TPA_EXPAND_GETELEMENTPTR_H
#define TPA_EXPAND_GETELEMENTPTR_H

#include "llvm/Pass.h"

class ExpandGetElementPtrPass: public llvm::BasicBlockPass
{
public:
	static char ID; // Pass identification, replacement for typeid
	ExpandGetElementPtrPass(): llvm::BasicBlockPass(ID) {}

	bool runOnBasicBlock(llvm::BasicBlock& BB) override;
};

#endif
