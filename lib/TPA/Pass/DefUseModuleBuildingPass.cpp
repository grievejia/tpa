#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseModuleBuilder.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/DefUseModuleBuildingPass.h"

using namespace llvm;

namespace tpa
{

bool DefUseModuleBuildingPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto const& ptrAnalysis = tpaWrapper.getPointerAnalysis();
	auto modRefTable = ExternalModRefTable::loadFromFile();
	ModRefModuleAnalysis modRefAnalysis(ptrAnalysis, modRefTable);
	auto summaryMap = modRefAnalysis.runOnModule(module);

	DefUseModuleBuilder builder(ptrAnalysis, summaryMap, modRefTable);
	auto duModule = builder.buildDefUseModule(module);

	duModule.writeDotFile("dots", "module");

	return false;
}

void DefUseModuleBuildingPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char DefUseModuleBuildingPass::ID = 0;
static RegisterPass<DefUseModuleBuildingPass> X("tpa-build-du-module", "DefUseModule builder backed up by TPA", true, true);

}
