#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "TPA/Pass/GlobalPointerAnalysisPass.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

bool GlobalPointerAnalysisPass::runOnModule(Module& module)
{
	auto ptrManager = PointerManager();
	auto dataLayout = DataLayout(&module);
	auto memManager = MemoryManager(dataLayout);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, memManager);
	auto envStore = globalAnalysis.runOnModule(module);
	auto env = std::move(envStore.first);
	auto store = std::move(envStore.second);

	errs() << env << store << "\n";

	return false;
}

void GlobalPointerAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char GlobalPointerAnalysisPass::ID = 0;
static RegisterPass<GlobalPointerAnalysisPass> X("global-pts", "Dump the global points-to info to the console", true, true);

}
