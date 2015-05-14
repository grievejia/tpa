#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/TunableModRefAnalysisPass.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

bool TunableModRefAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto modRefTable = ExternalModRefTable::loadFromFile();
	ModRefAnalysis modRefAnalysis(tpaWrapper.getPointerAnalysis(), modRefTable);
	auto summaryMap = modRefAnalysis.runOnProgram(tpaWrapper.getPointerProgram());

	summaryMap.dump(errs());

	return false;
}

void TunableModRefAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunableModRefAnalysisPass::ID = 0;
static RegisterPass<TunableModRefAnalysisPass> X("tpa-modref", "Mod/ref analysis backed up by TPA", true, true);

}
