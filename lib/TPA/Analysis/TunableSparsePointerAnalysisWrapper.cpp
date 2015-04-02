#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Analysis/TunableSparsePointerAnalysisWrapper.h"

using namespace llvm;

namespace tpa
{

TunableSparsePointerAnalysisWrapper::TunableSparsePointerAnalysisWrapper() {}
TunableSparsePointerAnalysisWrapper::~TunableSparsePointerAnalysisWrapper() {}

void TunableSparsePointerAnalysisWrapper::runOnModule(const llvm::Module& module)
{
	auto dataLayout = DataLayout(&module);
	memManager = std::make_unique<MemoryManager>(dataLayout);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, *memManager);

	auto initEnvStore = globalAnalysis.runOnModule(module);

	auto builder = SemiSparseProgramBuilder(extTable);
	auto prog = builder.buildSemiSparseProgram(module);

	// TPA initial pass
	TunablePointerAnalysis tpa(ptrManager, *memManager, extTable, initEnvStore.first);
	tpa.runOnProgram(prog, initEnvStore.second);

	auto extModTable = ExternalModTable();
	auto extRefTable = ExternalRefTable();
	ModRefAnalysis modRefAnalysis(tpa, extModTable, extRefTable);
	auto summaryMap = modRefAnalysis.runOnProgram(prog);

	DefUseProgramBuilder duBuilder(tpa, summaryMap, extModTable, extRefTable);
	dug = duBuilder.buildDefUseProgram(prog);

	TunableSparsePointerAnalysis stpa(ptrManager, *memManager, extTable, summaryMap, std::move(initEnvStore.first));
	stpa.runOnDefUseProgram(dug, std::move(initEnvStore.second));
}

}
