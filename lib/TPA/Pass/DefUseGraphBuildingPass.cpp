#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
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
	auto extModTable = ExternalModTable();
	auto extRefTable = ExternalRefTable();
	ModRefAnalysis modRefAnalysis(ptrAnalysis, extModTable, extRefTable);
	auto summaryMap = modRefAnalysis.runOnProgram(tpaWrapper.getPointerProgram());

	DefUseProgramBuilder builder(ptrAnalysis, summaryMap, extModTable, extRefTable);
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
