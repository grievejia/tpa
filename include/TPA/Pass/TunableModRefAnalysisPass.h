#ifndef TPA_TUNABLE_MOD_REF_ANALYSIS_PASS_H
#define TPA_TUNABLE_MOD_REF_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class TunableModRefAnalysisPass: public llvm::ModulePass
{
public:
	static char ID;
	TunableModRefAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
