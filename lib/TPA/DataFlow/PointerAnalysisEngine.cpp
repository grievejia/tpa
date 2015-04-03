#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"
#include "TPA/DataFlow/PointerCFGEvaluator.h"

using namespace llvm;

namespace tpa
{

PointerAnalysisEngine::PointerAnalysisEngine(PointerManager& p, MemoryManager& m, GlobalState<PointerProgram>& g, Store st, const ExternalPointerEffectTable& t): globalState(g), transferFunction(p, m, t)
{
	initializeWorkList(std::move(st));
}

void PointerAnalysisEngine::initializeWorkList(Store store)
{
	auto entryCtx = Context::getGlobalContext();
	auto entryCFG = globalState.getProgram().getEntryCFG();
	auto entryNode = entryCFG->getEntryNode();

	// Initialize workList and memo	
	globalState.getMemo().updateMemo(entryCtx, entryNode, store);
	globalWorkList.enqueue(entryCtx, entryCFG, entryNode);
}

void PointerAnalysisEngine::evalFunction(const Context* ctx, const PointerCFG* cfg)
{
	PointerCFGEvaluator evaluator(ctx, cfg, globalState, globalWorkList, transferFunction);
	evaluator.eval();
}

void PointerAnalysisEngine::run()
{
	while (!globalWorkList.isEmpty())
	{
		const Context* ctx;
		const PointerCFG* cfg;
		std::tie(ctx, cfg) = globalWorkList.dequeue();

		evalFunction(ctx, cfg);
	}
}

}
