#include "TPA/Analysis/TunableSparsePointerAnalysisWrapper.h"
#include "TPA/Pass/TunableSparsePointerAnalysisPass.h"

using namespace llvm;

namespace tpa
{

bool TunableSparsePointerAnalysisPass::runOnModule(Module& module)
{
	TunableSparsePointerAnalysisWrapper stpaWrapper;
	stpaWrapper.runOnModule(module);

	return false;
}

void TunableSparsePointerAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunableSparsePointerAnalysisPass::ID = 0;
static RegisterPass<TunableSparsePointerAnalysisPass> X("stpa", "Staged tunable pointer analysis pass", true, true);

}
