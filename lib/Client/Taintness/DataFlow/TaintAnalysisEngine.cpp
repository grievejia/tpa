#include "Client/Taintness/DataFlow/DefUseFunctionEvaluator.h"
#include "Client/Taintness/DataFlow/TaintAnalysisEngine.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TaintAnalysisEngine::TaintAnalysisEngine(TaintGlobalState& g, const ExternalPointerEffectTable& t): globalState(g), transferFunction(globalState.getPointerAnalysis(), t)
{
	initializeWorkList();
}

void TaintAnalysisEngine::initializeWorkList()
{
	// Find the entry function
	auto const& duModule = globalState.getProgram();
	auto& entryDefUseFunc = duModule.getEntryFunction();
	auto entryFunc = &entryDefUseFunc.getFunction();

	// Set up the initial environment
	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto globalCtx = Context::getGlobalContext();
	auto& env = globalState.getEnv();
	auto initStore = TaintStore();

	if (entryFunc->arg_size() > 0)
	{
		auto argcValue = entryFunc->arg_begin();
		env.strongUpdate(ProgramLocation(globalCtx, argcValue), TaintLattice::Tainted);
		if (entryFunc->arg_size() > 1)
		{
			auto argvValue = (++entryFunc->arg_begin());
			// TODO: is argv itself tainted or not? I would say no
			env.strongUpdate(ProgramLocation(globalCtx, argvValue), TaintLattice::Untainted);

			auto& memManager = ptrAnalysis.getMemoryManager();
			auto argvPtrLoc = memManager.getArgvPtrLoc();
			auto argvMemLoc = memManager.getArgvMemLoc();
			initStore.strongUpdate(argvPtrLoc, TaintLattice::Tainted);
			initStore.strongUpdate(argvMemLoc, TaintLattice::Tainted);
		}
	}

	// Initialize all externally-defined global values
	for (auto const& global: duModule.getModule().globals())
	{
		if (global.isDeclaration())
		{
			env.strongUpdate(ProgramLocation(globalCtx, &global), TaintLattice::Untainted);
			auto pSet = ptrAnalysis.getPtsSet(globalCtx, &global);
			for (auto loc: pSet)
				initStore.strongUpdate(loc, TaintLattice::Untainted);
		}
	}

	auto entryDuInst = entryDefUseFunc.getEntryInst();
	globalWorkList.enqueue(globalCtx, &entryDefUseFunc, entryDuInst);
	globalState.getMemo().update(ProgramLocation(globalCtx, entryDuInst->getInstruction()), std::move(initStore));
}

void TaintAnalysisEngine::evalFunction(const Context* ctx, const DefUseFunction* duFunc)
{
	DefUseFunctionEvaluator evaluator(ctx, duFunc, globalState, globalWorkList, transferFunction);
	evaluator.eval();
}

bool TaintAnalysisEngine::run()
{
	while (!globalWorkList.isEmpty())
	{
		const Context* ctx;
		const DefUseFunction* duFunc;
		std::tie(ctx, duFunc) = globalWorkList.dequeue();

		evalFunction(ctx, duFunc);
	}

	return !transferFunction.checkMemoStates(globalState.getEnv(), globalState.getMemo(), true);
}

}
}