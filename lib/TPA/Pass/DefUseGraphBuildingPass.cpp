#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/DefUseGraphBuildingPass.h"

using namespace llvm;

namespace tpa
{

bool DefUseGraphBuildingPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto const& ptrAnalysis = tpaWrapper.getPointerAnalysis();
	ModRefAnalysis modRefAnalysis(ptrAnalysis, tpaWrapper.getExtTable());
	auto summaryMap = modRefAnalysis.runOnProgram(tpaWrapper.getPointerProgram());

	DefUseProgramBuilder builder(ptrAnalysis, summaryMap, tpaWrapper.getExtTable());
	auto dug = builder.buildDefUseProgram(tpaWrapper.getPointerProgram());

	dug.writeDotFile("dots", "");

	return false;
}

void DefUseGraphBuildingPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char DefUseGraphBuildingPass::ID = 0;
static RegisterPass<DefUseGraphBuildingPass> X("tpa-build-dug", "DefUseGraph builder backed up by TPA", true, true);

}
