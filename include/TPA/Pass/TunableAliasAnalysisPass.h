#ifndef TPA_TUNABLE_ALIAS_ANALYSIS_PASS_H
#define TPA_TUNABLE_ALIAS_ANALYSIS_PASS_H

#include "TPA/Analysis/TunableAliasAnalysis.h"

#include <llvm/Pass.h>
#include <llvm/Analysis/AliasAnalysis.h>

namespace tpa
{

class TunableAliasAnalysisPass: public llvm::ModulePass, public llvm::AliasAnalysis
{
private:
	TunableAliasAnalysis taa;
public:
	static char ID;
	TunableAliasAnalysisPass(): llvm::ModulePass(ID) {}

	// LLVM Pass interfaces
	bool runOnModule(llvm::Module& M) override;
	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
	void releaseMemory() override;

	// LLVM AliasAnalysis interfaces
	AliasAnalysis::AliasResult alias(const AliasAnalysis::Location& l1, const AliasAnalysis::Location& l2) override;
	void* getAdjustedAnalysisPointer(llvm::AnalysisID PI) override;
};

}

#endif
