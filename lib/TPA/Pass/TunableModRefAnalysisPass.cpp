#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/TunableModRefAnalysisPass.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static void dumpModRefSummary(const ModRefSummaryMap& smap)
{
	errs() << "\n----- Mod-Ref Summary -----\n";
	for (auto const& mapping: smap)
	{
		errs() << "Function " << mapping.first->getName() << '\n';
		
		errs() << "\tvalue reads : { ";
		for (auto val: mapping.second.value_reads())
			errs() << val->getName() << " ";
		errs() << "}\n\tmem reads : { ";
		for (auto loc: mapping.second.mem_reads())
			errs() << *loc << " ";
		errs() << "}\n\tmem writes : { ";
		for (auto loc: mapping.second.mem_writes())
			errs() << *loc << " ";
		errs() << "}\n";
	}
	errs() << "----- End of Summary -----\n";
}

bool TunableModRefAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	ModRefAnalysis modRefAnalysis(tpaWrapper.getPointerAnalysis(), tpaWrapper.getExtTable());
	auto summaryMap = modRefAnalysis.runOnProgram(tpaWrapper.getPointerProgram());

	dumpModRefSummary(summaryMap);

	return false;
}

void TunableModRefAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunableModRefAnalysisPass::ID = 0;
static RegisterPass<TunableModRefAnalysisPass> X("tpa-modref", "Mod/ref analysis backed up by TPA", true, true);

}
