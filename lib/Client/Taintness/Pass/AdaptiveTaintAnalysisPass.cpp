using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

static bool doTaintAnalysis(const PointerAnalysis& pa, const DefUseModule& duModule, bool report = true)
{
	TaintAnalysis taintAnalysis(pa);
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
	
	if (doTaintAnalysis(tpa, duModule, false))
	{
		errs() << "--- second round ---\n";
		doTaintAnalysis(tpa, duModule, true);
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