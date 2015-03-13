#ifndef TPA_TUNABLE_ALIAS_ANALYSIS_PASS_H
#define TPA_TUNABLE_ALIAS_ANALYSIS_PASS_H

#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"

#include <llvm/Pass.h>
#include <llvm/Analysis/AliasAnalysis.h>

#include <memory>

namespace tpa
{

class AliasAnalysis;

class TunableAliasAnalysisPass: public llvm::ModulePass, public llvm::AliasAnalysis
{
private:
	TunablePointerAnalysisWrapper tpaWrapper;
	std::unique_ptr<tpa::AliasAnalysis> aa;
public:
	static char ID;
	TunableAliasAnalysisPass();
	~TunableAliasAnalysisPass();

	// LLVM Pass interfaces
	bool runOnModule(llvm::Module& M) override;
	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
	void releaseMemory() override;

	// LLVM AliasAnalysis interfaces
	llvm::AliasAnalysis::AliasResult alias(const AliasAnalysis::Location& l1, const AliasAnalysis::Location& l2) override;
	void* getAdjustedAnalysisPointer(llvm::AnalysisID PI) override;
};

}

#endif
