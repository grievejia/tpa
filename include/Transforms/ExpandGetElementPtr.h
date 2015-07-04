#pragma once

#include "llvm/Pass.h"

namespace transform
{

class ExpandGetElementPtrPass: public llvm::BasicBlockPass
{
public:
	static char ID; // Pass identification, replacement for typeid
	ExpandGetElementPtrPass(): llvm::BasicBlockPass(ID) {}

	bool runOnBasicBlock(llvm::BasicBlock& BB) override;
};

}
