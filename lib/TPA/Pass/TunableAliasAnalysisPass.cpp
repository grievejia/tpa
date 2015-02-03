#include "Pass/TunableAliasAnalysisPass.h"

using namespace llvm;

namespace tpa
{

bool TunableAliasAnalysisPass::runOnModule(llvm::Module& M)
{
	taa.runOnModule(M);
	return false;
}

AliasAnalysis::AliasResult TunableAliasAnalysisPass::alias(const AliasAnalysis::Location& l1, const AliasAnalysis::Location& l2)
{
	if (l1.Size == 0 || l2.Size == 0)
		return NoAlias;

	auto myRes = taa.globalAliasQuery(l1.Ptr, l2.Ptr);

	switch (myRes)
	{
		case tpa::AliasResult::NoAlias:
			return NoAlias;
		case tpa::AliasResult::MustAlias:
			return MustAlias;
		case tpa::AliasResult::MayAlias:
		//	return AliasAnalysis::alias(l1, l2);
			return MayAlias;
	}
}

void TunableAliasAnalysisPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
	AliasAnalysis::getAnalysisUsage(AU);
	AU.setPreservesAll();
}

void* TunableAliasAnalysisPass::getAdjustedAnalysisPointer(AnalysisID PI)
{
	if (PI == &AliasAnalysis::ID)
		return (AliasAnalysis *)this;
	return this;
}

void TunableAliasAnalysisPass::releaseMemory()
{
}

char TunableAliasAnalysisPass::ID = 0;
static RegisterPass<TunableAliasAnalysisPass> X("taa", "Alias analysis with tunable precision", true, true);
static RegisterAnalysisGroup<AliasAnalysis> Y(X);

}
