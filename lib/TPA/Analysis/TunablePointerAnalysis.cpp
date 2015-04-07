#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/DataFlow/GlobalState.h"
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
	GlobalState<PointerProgram> globalState(ptrManager, memManager, prog, callGraph, env, extTable);
	auto ptrEngine = PointerAnalysisEngine(globalState, std::move(store));
	ptrEngine.run();

	//errs() << env << '\n';
}

}
