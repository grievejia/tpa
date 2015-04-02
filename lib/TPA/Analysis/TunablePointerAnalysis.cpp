#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

TunablePointerAnalysis::TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerEffectTable& e): PointerAnalysis(p, m, e) {}
TunablePointerAnalysis::TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerEffectTable& e, const Env& en): PointerAnalysis(p, m, e, en) {}
TunablePointerAnalysis::TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerEffectTable& e, Env&& en): PointerAnalysis(p, m, e, std::move(en)) {}
TunablePointerAnalysis::~TunablePointerAnalysis() = default;

void TunablePointerAnalysis::runOnProgram(const PointerProgram& prog, Store store)
{
	auto ptrEngine = PointerAnalysisEngine(ptrManager, memManager, prog, env, std::move(store), callGraph, memo, extTable);
	ptrEngine.run();

	//env.dump(errs());
}

}
