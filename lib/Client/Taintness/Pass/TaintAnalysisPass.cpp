#include "Client/Taintness/Analysis/TaintAnalysis.h"
#include "Client/Taintness/Pass/TaintAnalysisPass.h"
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

bool TaintAnalysisPass::runOnModule(Module& module)
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

	TaintAnalysis taintAnalysis(tpa);

	auto hasError = taintAnalysis.runOnDefUseModule(duModule);
	if (!hasError)
	{
		errs().changeColor(raw_ostream::Colors::GREEN);
		errs() << "Program passed info flow check.\n";
		errs().resetColor();
	}
	
	return false;
}

void TaintAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TaintAnalysisPass::ID = 0;
static RegisterPass<TaintAnalysisPass> X("tpa-taint", "Taint analysis backed up by TPA", true, true);


}
}
