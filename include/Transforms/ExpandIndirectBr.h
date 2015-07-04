#pragma once

#include "llvm/Pass.h"

namespace transform
{

// This is a ModulePass so that it can expand out blockaddress
// ConstantExprs inside global variable initializers.
class ExpandIndirectBr: public llvm::ModulePass {
public:
	static char ID;
	ExpandIndirectBr(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module &M) override;
};

}
