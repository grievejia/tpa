#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"

using namespace llvm;

namespace tpa
{

TunablePointerAnalysisWrapper::TunablePointerAnalysisWrapper() {}
TunablePointerAnalysisWrapper::~TunablePointerAnalysisWrapper() {}

void TunablePointerAnalysisWrapper::runOnModule(const llvm::Module& module)
{
	assert(ptrAnalysis.get() == nullptr);

	auto dataLayout = DataLayout(&module);
	memManager = std::make_unique<MemoryManager>(dataLayout);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, *memManager);
	auto initEnvStore = globalAnalysis.runOnModule(module);
	auto initEnv = std::move(initEnvStore.first);
	auto initStore = std::move(initEnvStore.second);

	auto builder = SemiSparseProgramBuilder(extTable);
	prog = builder.buildSemiSparseProgram(module);

	ptrAnalysis = std::make_unique<TunablePointerAnalysis>(ptrManager, *memManager, extTable, std::move(initEnv));
	ptrAnalysis->runOnProgram(prog, std::move(initStore));

	//env.dump(errs());
}

}