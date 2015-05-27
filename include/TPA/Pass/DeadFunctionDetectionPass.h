#ifndef TPA_DEAD_FUNCTION_ELIMINATION_PASS_H
#define TPA_DEAD_FUNCTION_ELIMINATION_PASS_H

#include "llvm/Pass.h"

namespace tpa
{

class DeadFunctionDetectionPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	DeadFunctionDetectionPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;
	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
