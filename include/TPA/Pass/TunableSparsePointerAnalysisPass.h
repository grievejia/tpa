#ifndef TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_PASS_H
#define TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace tpa
{

class TunableSparsePointerAnalysisPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	TunableSparsePointerAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}

#endif
