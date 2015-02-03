#ifndef TPA_CFG_BUILDING_PASS_H
#define TPA_CFG_BUILDING_PASS_H

#include "llvm/Pass.h"

namespace tpa
{

class CFGBuildingPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	CFGBuildingPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

class SemiSparseCFGBuildingPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	SemiSparseCFGBuildingPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
