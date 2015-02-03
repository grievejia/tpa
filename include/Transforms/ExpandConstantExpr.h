#ifndef TPA_EXPAND_CONSTANT_EXPR_H
#define TPA_EXPAND_CONSTANT_EXPR_H

#include "llvm/Pass.h"

// This is a FunctionPass because our handling of PHI nodes means that our modifications may cross BasicBlocks.
class ExpandConstantExprPass: public llvm::FunctionPass
{
public:
	static char ID; // Pass identification, replacement for typeid
	ExpandConstantExprPass() : llvm::FunctionPass(ID) {}

	bool runOnFunction(llvm::Function& F) override;
};

#endif
