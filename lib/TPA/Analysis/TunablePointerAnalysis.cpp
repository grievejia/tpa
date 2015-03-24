#include "MemoryModel/PtsSet/StoreManager.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

TunablePointerAnalysis::TunablePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e): PointerAnalysis(p, m, ss.getPtsSetManager(), e), storeManager(ss), memo(storeManager) {}
TunablePointerAnalysis::TunablePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const Env& en): PointerAnalysis(p, m, ss.getPtsSetManager(), e, en), storeManager(ss), memo(storeManager) {}
TunablePointerAnalysis::TunablePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, Env&& en): PointerAnalysis(p, m, ss.getPtsSetManager(), e, std::move(en)), storeManager(ss), memo(storeManager) {}
TunablePointerAnalysis::~TunablePointerAnalysis() = default;

void TunablePointerAnalysis::runOnProgram(const PointerProgram& prog, Store store)
{
	auto ptrEngine = PointerAnalysisEngine(ptrManager, memManager, storeManager, prog, env, std::move(store), callGraph, memo, extTable);
	ptrEngine.run();

	//env.dump(errs());
}

}
