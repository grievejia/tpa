#ifndef TPA_TUNABLE_POINTER_ANALYSIS_PASS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class TunablePointerAnalysisPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	TunablePointerAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
