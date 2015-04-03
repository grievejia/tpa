#ifndef TPA_TAINT_ADAPTIVE_TAINT_ANALYSIS_PASS_H
#define TPA_TAINT_ADAPTIVE_TAINT_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace client
{
namespace taint
{

class AdaptiveTaintAnalysisPass: public llvm::ModulePass
{
public:
	static char ID;
	AdaptiveTaintAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}
}

#endif
