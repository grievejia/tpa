#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/ControlFlow/PointerCFGNodePrint.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/TunableReachingDefAnalysisPass.h"

using namespace llvm;

namespace tpa
{

static void dumpReachingDef(const PointerCFG& cfg, const ReachingDefMap<PointerCFGNode>& rdMap)
{
	errs() << "\n----- Reaching Def Summary -----\n";
	errs() << "Function: " << cfg.getFunction()->getName() << "\n";

	for (auto node: cfg)
	{
		if (isa<EntryNode>(node))
			continue;
		errs() << "{\n";
		auto& rdStore = rdMap.getReachingDefStore(node);
		for (auto const& mapping: rdStore)
		{
			errs() << "\t" << *mapping.first << " -> { ";
			for (auto node: mapping.second)
				errs() << *node << " ";
			errs() << "}\n";
		}
		errs() << "}\n";
		errs() << "\n" << *node << "\n";
	}
	
	errs() << "----- End of Summary -----\n";
}

bool TunableReachingDefAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto extModTable = ExternalModTable();
	auto extRefTable = ExternalRefTable();
	ModRefAnalysis modRefAnalysis(tpaWrapper.getPointerAnalysis(), extModTable, extRefTable);
	auto summaryMap = modRefAnalysis.runOnProgram(tpaWrapper.getPointerProgram());

	ReachingDefAnalysis rdAnalysis(tpaWrapper.getPointerAnalysis(), summaryMap, extModTable);
	for (auto const& cfg: tpaWrapper.getPointerProgram())
	{
		auto rdMap = rdAnalysis.runOnPointerCFG(cfg);
		dumpReachingDef(cfg, rdMap);
	}


	return false;
}

void TunableReachingDefAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunableReachingDefAnalysisPass::ID = 0;
static RegisterPass<TunableReachingDefAnalysisPass> X("tpa-reaching-def", "Reaching def analysis backed up by TPA", true, true);

}
