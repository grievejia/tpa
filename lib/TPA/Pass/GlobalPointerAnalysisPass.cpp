#include "Analysis/GlobalPointerAnalysis.h"
#include "DataFlow/StoreManager.h"
#include "Memory/MemoryManager.h"
#include "Memory/PointerManager.h"
#include "Pass/GlobalPointerAnalysisPass.h"

#include <llvm/IR/DataLayout.h>
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

	VectorSetManager<const MemoryLocation*> pSetManager;
	auto storeManager = StoreManager(pSetManager);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, memManager, storeManager);
	auto envStore = globalAnalysis.runOnModule(module);
	auto env = std::move(envStore.first);
	auto store = std::move(envStore.second);

	errs() << "---- Env ----\n";
	for (auto const& mapping: env)
	{
		errs() << *mapping.first << "  -->  { ";
		for (auto loc: *mapping.second)
			errs() << *loc << " ";
		errs() << "}\n";
	}

	errs() << "\n---- Store ----\n";
	for (auto const& mapping: store.getGlobalStore())
	{
		errs() << *mapping.first << "  -->  { ";
		for (auto loc: *mapping.second)
			errs() << *loc << " ";
		errs() << "}\n";
	}

	return false;
}

void GlobalPointerAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char GlobalPointerAnalysisPass::ID = 0;
static RegisterPass<GlobalPointerAnalysisPass> X("global-pts", "Dump the global points-to info to the console", true, true);

}
