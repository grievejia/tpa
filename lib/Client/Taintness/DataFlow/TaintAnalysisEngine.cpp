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

TaintAnalysisEngine::TaintAnalysisEngine(TaintGlobalState& g): globalState(g)
{
	initializeWorkList();
}

void TaintAnalysisEngine::initializeMainArgs(TaintEnv& env, TaintStore& store, const DefUseFunction& entryDuFunc)
{
	auto entryFunc = &entryDuFunc.getFunction();
	auto globalCtx = Context::getGlobalContext();
	auto const& ptrAnalysis = globalState.getPointerAnalysis();

	if (entryFunc->arg_size() > 0)
	{
		// Process argc
		auto argcValue = entryFunc->arg_begin();
		env.strongUpdate(ProgramLocation(globalCtx, argcValue), TaintLattice::Tainted);

		// Process argv, *argv and **argv
		if (entryFunc->arg_size() > 1)
		{
			// argv is not tainted
			auto argvValue = (++entryFunc->arg_begin());
			env.strongUpdate(ProgramLocation(globalCtx, argvValue), TaintLattice::Untainted);

			// *argv is tainted
			auto& memManager = ptrAnalysis.getMemoryManager();
			auto argvPtrLoc = memManager.getArgvPtrLoc();
			store.strongUpdate(argvPtrLoc, TaintLattice::Tainted);

			// **argv is definitely tainted
			auto argvMemLoc = memManager.getArgvMemLoc();
			store.strongUpdate(argvMemLoc, TaintLattice::Tainted);
		}
	}
}

void TaintAnalysisEngine::initializeExternalGlobalValues(TaintEnv& env, TaintStore& store, const DefUseModule& duModule)
{
	auto globalCtx = Context::getGlobalContext();
	auto const& ptrAnalysis = globalState.getPointerAnalysis();

	for (auto const& global: duModule.getModule().globals())
	{
		if (global.isDeclaration())
		{
			auto pSet = ptrAnalysis.getPtsSet(globalCtx, &global);
			for (auto loc: pSet)
				store.strongUpdate(loc, TaintLattice::Untainted);
		}
	}
}

void TaintAnalysisEngine::enqueueEntryPoint(const DefUseFunction& entryDuFunc, TaintStore store)
{
	auto globalCtx = Context::getGlobalContext();
	auto entryDuInst = entryDuFunc.getEntryInst();

	globalWorkList.enqueue(globalCtx, &entryDuFunc, entryDuInst);
	globalState.getMemo().update(DefUseProgramLocation(globalCtx, entryDuInst), std::move(store));
}

void TaintAnalysisEngine::initializeWorkList()
{
	// Find the entry function
	auto const& duModule = globalState.getProgram();
	auto& entryDefUseFunc = duModule.getEntryFunction();

	// Set up the initial environment
	auto& env = globalState.getEnv();
	auto initStore = TaintStore();

	initializeMainArgs(env, initStore, entryDefUseFunc);
	initializeExternalGlobalValues(env, initStore, duModule);
	enqueueEntryPoint(entryDefUseFunc, std::move(initStore));
}

void TaintAnalysisEngine::run()
{
	while (!globalWorkList.empty())
	{
		const Context* ctx;
		const DefUseFunction* duFunc;
		std::tie(ctx, duFunc) = globalWorkList.dequeue();

		DefUseFunctionEvaluator(ctx, duFunc, globalState, globalWorkList).eval();
	}
}

}
}