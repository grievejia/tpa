#ifndef TPA_GLOBAL_POINTER_ANALYSIS_PASS_H
#define TPA_GLOBAL_POINTER_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class GlobalPointerAnalysisPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	GlobalPointerAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
