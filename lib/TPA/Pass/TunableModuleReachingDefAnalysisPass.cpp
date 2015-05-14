#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefModuleAnalysis.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"
#include "TPA/Pass/TunableModuleReachingDefAnalysisPass.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static void dumpReachingDef(const Function& f, const ReachingDefMap<Instruction>& rdMap)
{
	errs() << "\n----- Reaching Def Summary -----\n";
	errs() << "Function: " << f.getName() << "\n";

	for (auto const& bb: f)
	{
		for (auto const& inst: bb)
		{
			errs() << "{\n";
			auto& rdStore = rdMap.getReachingDefStore(&inst);
			for (auto const& mapping: rdStore)
			{
				errs() << "\t" << *mapping.first << " -> { ";
				for (auto inst: mapping.second)
				{
					if (inst == nullptr)
						errs() << "EXTERNAL ";
					else
						errs() << *inst << " ";
				}
				errs() << "}\n";
			}
			errs() << "}\n";
			errs() << "\n" << inst << "\n";
		}
	}
	
	errs() << "----- End of Summary -----\n";
}

bool TunableModuleReachingDefAnalysisPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto modRefTable = ExternalModRefTable::loadFromFile();
	ModRefModuleAnalysis modRefAnalysis(tpaWrapper.getPointerAnalysis(), modRefTable);
	auto summaryMap = modRefAnalysis.runOnModule(module);

	ReachingDefModuleAnalysis rdAnalysis(tpaWrapper.getPointerAnalysis(), summaryMap, modRefTable);
	for (auto const& f: module)
	{
		if (f.isDeclaration())
			continue;
		auto rdMap = rdAnalysis.runOnFunction(f);
		dumpReachingDef(f, rdMap);
	}

	return false;
}

void TunableModuleReachingDefAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char TunableModuleReachingDefAnalysisPass::ID = 0;
static RegisterPass<TunableModuleReachingDefAnalysisPass> X("tpa-module-reaching-def", "Reaching def analysis on llvm::Module backed up by TPA", true, true);

}
