#include "MemoryModel/Precision/KLimitContext.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Pass/TunablePointerAnalysisPass.h"

#include <llvm/Support/CommandLine.h>

using namespace llvm;

static cl::opt<unsigned, true> TPAContextSensitivity("k", cl::desc("In k-limit stack-k-CFA settings, this is the value of k"), cl::location(tpa::KLimitContext::defaultLimit), cl::init(0));

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
