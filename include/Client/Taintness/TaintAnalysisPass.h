#ifndef TPA_TAINT_ANALYSIS_PASS_H
#define TPA_TAINT_ANALYSIS_PASS_H

#include <llvm/Pass.h>

namespace client
{
namespace taint
{

class TaintAnalysisPass: public llvm::ModulePass
{
public:
	static char ID;
	TaintAnalysisPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}
}

#endif
