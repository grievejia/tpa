#include "MemoryModel/Precision/KLimitContext.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/TunablePointerAnalysisPass.h"

using namespace llvm;

namespace tpa
{

bool TunablePointerAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	return false;
}

void TunablePointerAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunablePointerAnalysisPass::ID = 0;
static RegisterPass<TunablePointerAnalysisPass> X("tpa", "Tunable pointer analysis single pass", true, true);

}
