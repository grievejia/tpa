#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/TunableModuleModRefAnalysisPass.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

bool TunableModuleModRefAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	ModRefModuleAnalysis modRefAnalysis(tpaWrapper.getPointerAnalysis(), ExternalModTable(), ExternalRefTable());
	auto summaryMap = modRefAnalysis.runOnModule(module);

	summaryMap.dump(errs());

	return false;
}

void TunableModuleModRefAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunableModuleModRefAnalysisPass::ID = 0;
static RegisterPass<TunableModuleModRefAnalysisPass> X("tpa-modref-module", "Mod/ref analysis for LLVM module backed up by TPA", true, true);

}
