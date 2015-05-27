#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/DeadFunctionDetectionPass.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static void searchForDeadFunction(const Function& f, DenseSet<const Function*>& liveFuncs, const PointerAnalysis& ptrAnalysis)
{
	for (auto const& bb: f)
	{
		for (auto const& inst: bb)
		{
			ImmutableCallSite cs(&inst);
			if (!cs)
				continue;

			auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), &inst);
			liveFuncs.insert(callTgts.begin(), callTgts.end());
		}
	}
}

bool DeadFunctionDetectionPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto const& ptrAnalysis = tpaWrapper.getPointerAnalysis();
	
	auto allFuncs = DenseSet<const Function*>();
	for (auto const& f: module)
		allFuncs.insert(&f);

	bool changed;
	do
	{
		changed = false;
		auto liveFuncs = DenseSet<const Function*>();
		for (auto f: allFuncs)
			searchForDeadFunction(*f, liveFuncs, ptrAnalysis);

		liveFuncs.insert(module.getFunction("main"));
		if (liveFuncs.size() < allFuncs.size())
		{
			allFuncs = std::move(liveFuncs);
			changed = true;
		}
	} while (changed);

	for (auto const& f: module)
	{
		if (!allFuncs.count(&f) && f.getName() != "main")
			errs() << "Dead function: " << f.getName() << "\n";
	}

	errs() << "To remove them:\n";
	errs() << "llvm-extract -delete";
	for (auto const& f: module)
	{
		if (!allFuncs.count(&f) && f.getName() != "main")
			errs() << " -func=" << f.getName();
	}
	errs() << " " << module.getName() << "\n";

	return false;
}

void DeadFunctionDetectionPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char DeadFunctionDetectionPass::ID = 0;
static RegisterPass<DeadFunctionDetectionPass> X("tpa-dead-func-detect", "Detect additional dead functions based on the analysis result of TPA", true, true);

}