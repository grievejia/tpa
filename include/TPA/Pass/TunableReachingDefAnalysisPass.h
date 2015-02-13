#ifndef TPA_TUNABLE_REACHING_DEF_ANALYSIS_PASS_H
#define TPA_TUNABLE_REACHING_DEF_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class TunableReachingDefAnalysisPass: public llvm::ModulePass
{
public:
	static char ID;
	TunableReachingDefAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
