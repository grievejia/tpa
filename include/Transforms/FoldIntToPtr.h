#pragma once

#include "llvm/Pass.h"

namespace transform
{

class FoldIntToPtrPass: public llvm::BasicBlockPass {
public:
	static char ID;
	FoldIntToPtrPass(): llvm::BasicBlockPass(ID) {}

	bool runOnBasicBlock(llvm::BasicBlock&) override;
};

}