#ifndef TPA_DEF_USE_MODULE_BUILDING_PASS_H
#define TPA_DEF_USE_MODULE_BUILDING_PASS_H

#include "llvm/Pass.h"

namespace tpa
{

class DefUseModuleBuildingPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	DefUseModuleBuildingPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
