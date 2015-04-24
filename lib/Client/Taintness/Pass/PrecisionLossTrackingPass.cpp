#include "Client/Taintness/Analysis/PrecisionLossTrackingAnalysis.h"
#include "Client/Taintness/Pass/PrecisionLossTrackingPass.h"
#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseModuleBuilder.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

bool PrecisionLossTrackingPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);
	auto& tpa = tpaWrapper.getPointerAnalysis();

	auto extModTable = ExternalModTable();
	auto extRefTable = ExternalRefTable();
	ModRefModuleAnalysis modRefAnalysis(tpa, extModTable, extRefTable);
	auto summaryMap = modRefAnalysis.runOnModule(module);

	DefUseModuleBuilder duBuilder(tpa, summaryMap, extModTable, extRefTable);
	auto duModule = duBuilder.buildDefUseModule(module);

	PrecisionLossTrackingAnalysis analysis(tpa);

	analysis.runOnDefUseModule(duModule);
	
	return false;
}

void PrecisionLossTrackingPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char PrecisionLossTrackingPass::ID = 0;
static RegisterPass<PrecisionLossTrackingPass> X("loss-track", "Taint analysis precision loss tracker backed up by TPA", true, true);

}
}