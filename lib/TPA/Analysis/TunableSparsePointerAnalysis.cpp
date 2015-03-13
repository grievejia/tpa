#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/Analysis/TunableSparsePointerAnalysis.h"
#include "TPA/DataFlow/SparseAnalysisEngine.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>

using namespace llvm;

namespace tpa
{

TunableSparsePointerAnalysis::TunableSparsePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const ModRefSummaryMap& sm): PointerAnalysis(p, m, ss.getPtsSetManager(), e), storeManager(ss), summaryMap(sm), memo(storeManager) {}
TunableSparsePointerAnalysis::TunableSparsePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const ModRefSummaryMap& sm, const Env& en): PointerAnalysis(p, m, ss.getPtsSetManager(), e, en), storeManager(ss), summaryMap(sm), memo(storeManager) {}
TunableSparsePointerAnalysis::TunableSparsePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const ModRefSummaryMap& sm, Env&& en): PointerAnalysis(p, m, ss.getPtsSetManager(), e, std::move(en)), storeManager(ss), summaryMap(sm), memo(storeManager) {}
TunableSparsePointerAnalysis::~TunableSparsePointerAnalysis() {}

void TunableSparsePointerAnalysis::runOnDefUseProgram(const DefUseProgram& dug, Store store)
{
	auto sparseEngine = SparseAnalysisEngine(ptrManager, memManager, storeManager, callGraph, memo, summaryMap, extTable);
	sparseEngine.runOnDefUseProgram(dug, env, std::move(store));
}

}
