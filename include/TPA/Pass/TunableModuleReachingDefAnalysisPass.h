#ifndef TPA_TUNABLE_MODULE_REACHING_DEF_ANALYSIS_PASS_H
#define TPA_TUNABLE_MODULE_REACHING_DEF_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class TunableModuleReachingDefAnalysisPass: public llvm::ModulePass
{
public:
	static char ID;
	TunableModuleReachingDefAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
