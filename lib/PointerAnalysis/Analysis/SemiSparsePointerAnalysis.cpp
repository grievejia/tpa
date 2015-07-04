#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/Initializer.h"
#include "PointerAnalysis/Engine/SemiSparsePropagator.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/Program/SemiSparseProgram.h"
#include "Util/AnalysisEngine/DataFlowAnalysis.h"

namespace tpa
{

void SemiSparsePointerAnalysis::runOnProgram(const SemiSparseProgram& ssProg)
{
	auto initStore = Store();
	std::tie(env, initStore) = GlobalPointerAnalysis(ptrManager, memManager, ssProg.getTypeMap()).runOnModule(ssProg.getModule());

	auto globalState = GlobalState(ptrManager, memManager, ssProg, extTable, env);
	auto dfa = util::DataFlowAnalysis<GlobalState, Memo, TransferFunction, SemiSparsePropagator>(globalState, memo);
	dfa.runOnInitialState<Initializer>(std::move(initStore));
}

PtsSet SemiSparsePointerAnalysis::getPtsSetImpl(const Pointer* ptr) const
{
	return env.lookup(ptr);
}

}