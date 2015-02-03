#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Pass/TunablePointerAnalysisPass.h"

using namespace llvm;

namespace tpa
{

bool TunablePointerAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysis tpaAnalysis;
	tpaAnalysis.runOnModule(module);

	return false;
}

void TunablePointerAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunablePointerAnalysisPass::ID = 0;
static RegisterPass<TunablePointerAnalysisPass> X("tpa", "Tunable pointer analysis single pass", true, true);

}
