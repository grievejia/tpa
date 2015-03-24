#ifndef TPA_MODULE_MOD_REF_ANALYSIS_PASS_H
#define TPA_MODULE_MOD_REF_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class TunableModuleModRefAnalysisPass: public llvm::ModulePass
{
public:
	static char ID;
	TunableModuleModRefAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
