#ifndef TPA_PRECISION_LOSS_TRACKING_PASS_H
#define TPA_PRECISION_LOSS_TRACKING_PASS_H

#include <llvm/Pass.h>

namespace client
{
namespace taint
{

class PrecisionLossTrackingPass: public llvm::ModulePass
{
public:
	static char ID;
	PrecisionLossTrackingPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

}
}

#endif
