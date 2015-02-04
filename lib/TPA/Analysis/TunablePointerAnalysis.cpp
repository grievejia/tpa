#include "MemoryModel/Analysis/GlobalPointerAnalysis.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/ControlFlow/SemiSparseProgramBuilder.h"
#include "TPA/DataFlow/ExternalPointerEffectTable.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"
#include "MemoryModel/Memory/MemoryManager.h"

#include <llvm/IR/DataLayout.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

TunablePointerAnalysis::TunablePointerAnalysis(): storeManager(pSetManager), env(pSetManager), memo(storeManager) {}
TunablePointerAnalysis::~TunablePointerAnalysis() = default;

void TunablePointerAnalysis::runOnModule(const llvm::Module& module)
{
	auto dataLayout = DataLayout(&module);
	memManager = std::make_unique<MemoryManager>(dataLayout);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, *memManager, storeManager);

	auto initEnvStore = globalAnalysis.runOnModule(module);
	env = std::move(initEnvStore.first);
	auto initStore = std::move(initEnvStore.second);

	auto extTable = ExternalPointerEffectTable();
	auto builder = SemiSparseProgramBuilder(extTable);
	auto prog = builder.buildSemiSparseProgram(module);

	auto ptrEngine = PointerAnalysisEngine(ptrManager, *memManager, storeManager, callGraph, memo, extTable);
	ptrEngine.runOnProgram(prog, env, std::move(initStore));

	for (auto const& mapping: env)
	{
		errs() << *mapping.first << " --> { ";
		for (auto loc: *mapping.second)
			errs() << *loc << " ";
		errs() << "}\n";
	}
}

}
