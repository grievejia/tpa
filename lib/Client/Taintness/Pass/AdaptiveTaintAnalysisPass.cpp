#include "Client/Taintness/Analysis/TaintAnalysis.h"
#include "Client/Taintness/Pass/AdaptiveTaintAnalysisPass.h"
#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseModuleBuilder.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

static bool doTaintAnalysis(const PointerAnalysis& pa, const ExternalPointerEffectTable& extTable, const DefUseModule& duModule, bool report = true)
{
	TaintAnalysis taintAnalysis(pa, extTable);
	auto hasError = taintAnalysis.runOnDefUseModule(duModule, report);
	if (!hasError)
	{
		errs().changeColor(raw_ostream::Colors::GREEN);
		errs() << "Program passed info flow check.\n";
		errs().resetColor();
		return false;
	}
	taintAnalysis.adaptContextSensitivity();
	return true;
}

bool AdaptiveTaintAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);
	auto& tpa = tpaWrapper.getPointerAnalysis();
	auto& extTable = tpaWrapper.getExtTable();

	auto extModTable = ExternalModTable();
	auto extRefTable = ExternalRefTable();
	ModRefModuleAnalysis modRefAnalysis(tpa, extModTable, extRefTable);
	auto summaryMap = modRefAnalysis.runOnModule(module);

	DefUseModuleBuilder duBuilder(tpa, summaryMap, extModTable, extRefTable);
	auto duModule = duBuilder.buildDefUseModule(module);
	
	if (doTaintAnalysis(tpa, extTable, duModule, false))
	{
		errs() << "--- second round ---\n";
		doTaintAnalysis(tpa, extTable, duModule, true);
	}

	return false;
}

void AdaptiveTaintAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char AdaptiveTaintAnalysisPass::ID = 0;
static RegisterPass<AdaptiveTaintAnalysisPass> X("tpa-taint-adaptive", "Adaptive taint analysis backed up by TPA", true, true);

}
}