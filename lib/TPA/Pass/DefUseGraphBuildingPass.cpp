#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Pass/DefUseGraphBuildingPass.h"

using namespace llvm;

namespace tpa
{

bool DefUseGraphBuildingPass::runOnModule(Module& module)
{
	TunablePointerAnalysis tpaAnalysis;
	tpaAnalysis.runOnModule(module);

	ExternalPointerEffectTable extTable;
	ModRefAnalysis modRefAnalysis(tpaAnalysis, extTable);
	auto summaryMap = modRefAnalysis.runOnProgram(tpaAnalysis.getPointerProgram());

	DefUseProgramBuilder builder(tpaAnalysis, summaryMap, extTable);
	auto dug = builder.buildDefUseProgram(tpaAnalysis.getPointerProgram());

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
