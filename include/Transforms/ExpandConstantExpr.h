#pragma once

#include "llvm/Pass.h"

namespace transform
{

// This is a FunctionPass because our handling of PHI nodes means that our modifications may cross BasicBlocks.
class ExpandConstantExprPass: public llvm::FunctionPass
{
public:
	static char ID; // Pass identification, replacement for typeid
	ExpandConstantExprPass() : llvm::FunctionPass(ID) {}

	bool runOnFunction(llvm::Function& F) override;
};

}
