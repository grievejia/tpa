#include "Context/Context.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "TaintAnalysis/Engine/TaintGlobalState.h"
#include "TaintAnalysis/Engine/Initializer.h"
#include "TaintAnalysis/Program/DefUseModule.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintMemo.h"

#include <llvm/IR/Function.h>

using namespace context;

namespace taint
{

Initializer::Initializer(TaintGlobalState& g, TaintMemo& m): duModule(g.getDefUseModule()), env(g.getEnv()), ptrAnalysis(g.getPointerAnalysis()), memo(m)
{
}

void Initializer::initializeMainArgs(TaintStore& store)
{
	auto const& entryFunc = duModule.getEntryFunction().getFunction();
	auto globalCtx = Context::getGlobalContext();
	
	if (entryFunc.arg_size() > 0)
	{
		// argc is tainted
		auto argcValue = entryFunc.arg_begin();
		env.strongUpdate(TaintValue(globalCtx, argcValue), TaintLattice::Tainted);

		if (entryFunc.arg_size() > 1)
		{
			// argv is not tainted
			auto argvValue = (++entryFunc.arg_begin());
			env.strongUpdate(TaintValue(globalCtx, argvValue), TaintLattice::Untainted);

			// *argv and **argv are tainted
			auto argvObj = ptrAnalysis.getMemoryManager().getArgvObject();
			store.strongUpdate(argvObj, TaintLattice::Tainted);

			if (entryFunc.arg_size() > 2)
			{
				auto envpValue = (++argvValue);
				env.strongUpdate(TaintValue(globalCtx, envpValue), TaintLattice::Untainted);

				auto envpObj = ptrAnalysis.getMemoryManager().getEnvpObject();
				store.strongUpdate(envpObj, TaintLattice::Tainted);
			}
		}
	}
}

void Initializer::initializeGlobalVariables(TaintStore& store)
{
	auto globalCtx = Context::getGlobalContext();
	for (auto const& global: duModule.getModule().globals())
	{
		// We don't need to worry about global variables in env because all of them are constants and are already handled properly

		auto pSet = ptrAnalysis.getPtsSet(globalCtx, &global);
		for (auto obj: pSet)
		{
			if (!obj->isSpecialObject())
				store.strongUpdate(obj, TaintLattice::Untainted);
		}
	}
	store.strongUpdate(tpa::MemoryManager::getUniversalObject(), TaintLattice::Either);
}

WorkList Initializer::runOnInitState(TaintStore&& initStore)
{
	WorkList workList;

	initializeMainArgs(initStore);
	initializeGlobalVariables(initStore);

	auto entryCtx = context::Context::getGlobalContext();
	auto entryInst = duModule.getEntryFunction().getEntryInst();
	assert(entryInst != nullptr);
	
	auto pp = ProgramPoint(entryCtx, entryInst);
	memo.update(pp, std::move(initStore));
	workList.enqueue(pp);

	return workList;
}

}