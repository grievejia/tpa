#include "Client/Taintness/TaintAnalysis.h"
#include "Client/Taintness/TaintAnalysisPass.h"
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

	TaintAnalysis taintAnalysis(tpaWrapper.getPointerAnalysis());
	auto hasError = taintAnalysis.runOnModule(module);
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
