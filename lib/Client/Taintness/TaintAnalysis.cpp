#include "Client/Taintness/TaintAnalysis.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Precision/AdaptiveContext.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TaintAnalysis::ClientWorkList TaintAnalysis::initializeWorkList(const DefUseModule& duModule)
{
	auto worklist = ClientWorkList();

	// Find the entry function
	auto& entryDefUseFunc = duModule.getEntryFunction();
	auto entryFunc = &entryDefUseFunc.getFunction();

	// Set up the initial environment
	auto globalCtx = Context::getGlobalContext();
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
			if (auto pSet = ptrAnalysis.getPtsSet(globalCtx, &global))
			{
				for (auto loc: *pSet)
					initStore.strongUpdate(loc, TaintLattice::Untainted);
			}
		}
	}

	worklist.enqueue(globalCtx, &entryDefUseFunc);
	auto& entryWorkList = worklist.getLocalWorkList(globalCtx, &entryDefUseFunc);
	propagateStateChange(globalCtx, entryDefUseFunc.getEntryInst(), true, true, initStore, entryWorkList);
	return worklist;
}

void TaintAnalysis::applyFunction(const Context* oldCtx, const Context* newCtx, ImmutableCallSite cs, const DefUseFunction* duFunc, const TaintStore& store, ClientWorkList& funWorkList)
{
	auto f = &duFunc->getFunction();
	assert(!f->isDeclaration());

	assert(cs.arg_size() >= f->arg_size());
	std::vector<TaintLattice> callerVals;
	callerVals.reserve(f->arg_size());
	for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
	{
		auto arg = cs.getArgument(i);
		auto argVal = TaintLattice::Untainted;
		if (!isa<Constant>(arg))
		{
			auto optArgVal = env.lookup(ProgramLocation(oldCtx, arg));
			if (!optArgVal)
				return;
			argVal = *optArgVal;
		}

		callerVals.push_back(argVal);
	}

	monitor.trackCallSite(ProgramLocation(oldCtx, cs.getInstruction()), f, newCtx, env, callerVals);

	bool envChanged = false;
	auto paramItr = f->arg_begin();
	for (auto argVal: callerVals)
	{
		envChanged |= env.weakUpdate(ProgramLocation(newCtx, paramItr), argVal);
		++paramItr;
	}

	envChanged |= visitedFuncs.insert(ProgramLocation(newCtx, f)).second;
	auto entryDefUseInst = duFunc->getEntryInst();
	if (envChanged)
	{
		funWorkList.enqueue(newCtx, duFunc);
		auto& tgtLocalWorkList = funWorkList.getLocalWorkList(newCtx, duFunc);
		for (auto succ: entryDefUseInst->top_succs())
			tgtLocalWorkList.enqueue(succ);
	}
	for (auto const& mapping: entryDefUseInst->mem_succs())
	{
		auto loc = mapping.first;
		auto optVal = store.lookup(loc);
		if (optVal)
		{
			for (auto succ: mapping.second)
			{
				if (insertMemoState(ProgramLocation(newCtx, succ->getInstruction()), loc, *optVal))
					funWorkList.enqueue(newCtx, duFunc, succ);
			}
		}
	}
}

void TaintAnalysis::evalReturn(const tpa::Context* ctx, const llvm::Instruction* inst, TaintEnv& env, const TaintStore& store, const DefUseModule& duModule, ClientWorkList& funWorkList)
{
	auto retInst = cast<ReturnInst>(inst);

	auto itr = retMap.find(std::make_pair(ctx, inst->getParent()->getParent()));
	assert(itr != retMap.end());
	auto const& returnTgts = itr->second;

	auto hasReturnVal = false;
	auto retVal = TaintLattice::Untainted;
	if (auto ret = retInst->getReturnValue())
	{
		hasReturnVal = true;
		if (!isa<Constant>(ret))
		{
			auto optRetVal = env.lookup(ProgramLocation(ctx, ret));
			if (!optRetVal)
				return;
			retVal = *optRetVal;
		}
	}

	for (auto retTgt: returnTgts)
	{
		auto oldCtx = retTgt.first;
		auto oldInst = retTgt.second;
		auto oldFunc = oldInst->getParent()->getParent();
		auto& oldDuFunc = duModule.getDefUseFunction(oldFunc);

		auto envChanged = false;
		if (hasReturnVal)
			envChanged |= env.weakUpdate(ProgramLocation(oldCtx, oldInst), retVal);

		funWorkList.enqueue(oldCtx, &oldDuFunc);
		auto& oldWorkList = funWorkList.getLocalWorkList(oldCtx, &oldDuFunc);
		propagateStateChange(oldCtx, oldDuFunc.getDefUseInstruction(oldInst), envChanged, true, store, oldWorkList);
	}
}

void TaintAnalysis::propagateStateChange(const Context* ctx, const DefUseInstruction* duInst, bool envChanged, bool storeChanged, const TaintStore& store, ClientWorkList::LocalWorkList& workList)
{
	// Propagate top-level value changes
	if (envChanged)
	{
		for (auto succ: duInst->top_succs())
			workList.enqueue(succ);
	}

	// Propagate memory-level changes
	if (storeChanged)
	{
		for (auto const& mapping: duInst->mem_succs())
		{
			auto usedLoc = mapping.first;
			auto optLocVal = store.lookup(usedLoc);
			if (!optLocVal)
				continue;
			//assert(optLocVal);
			for (auto succ: mapping.second)
			{
				if (insertMemoState(ProgramLocation(ctx, succ->getInstruction()), usedLoc, *optLocVal))
				{
					workList.enqueue(succ);
				}
			}
		}
	}
}

void TaintAnalysis::evalFunction(const Context* ctx, const DefUseFunction* duFunc, ClientWorkList& funWorkList, const DefUseModule& duModule)
{
	errs() << "Context = " << *ctx << "\n";
	errs() << "Function = " << duFunc->getFunction().getName() << "\n";
	auto& workList = funWorkList.getLocalWorkList(ctx, duFunc);

	while (!workList.isEmpty())
	{
		auto duInst = workList.dequeue();
		auto inst = duInst->getInstruction();

		//errs() << "inst = " << *inst << "\n";
		auto itr = memo.find(ProgramLocation(ctx, inst));
		auto const& store = (itr == memo.end()) ? TaintStore() : itr->second;

		if (isa<CallInst>(inst) || isa<InvokeInst>(inst))
		{
			ImmutableCallSite cs(inst);
			assert(cs && "evalCall() gets an non-call?");

			auto callees = ptrAnalysis.getCallGraph().getCallTargets(std::make_pair(ctx, inst));

			for (auto callTgt: callees)
			{
				auto callee = callTgt.second;
				if (callee->isDeclaration())
				{
					bool isValid, envChanged, storeChanged;
					auto newStore = store;
					std::tie(isValid, envChanged, storeChanged) = transferFunction.processLibraryCall(ctx, callee, cs, env, newStore);
					if (!isValid)
						continue;
					propagateStateChange(ctx, duInst, envChanged, storeChanged, newStore, workList);
				}
				else
				{
					auto newCtx = AdaptiveContext::pushContext(ctx, inst, callee);
					retMap[std::make_pair(newCtx, callee)].insert(std::make_pair(ctx, inst));
					applyFunction(ctx, newCtx, cs, &duModule.getDefUseFunction(callee), store, funWorkList);
				}
			}
		}
		else if (isa<ReturnInst>(inst))
		{
			if (inst->getParent()->getParent()->getName() == "main")
			{
				errs() << "T Reached Program end\n";
				continue;
			}

			evalReturn(ctx, inst, env, store, duModule, funWorkList);
		}
		else
		{
			bool isValid, envChanged, storeChanged;
			auto newStore = store;
			std::tie(isValid, envChanged, storeChanged) = transferFunction.evalInst(ctx, inst, env, newStore);
			if (!isValid)
				continue;
			propagateStateChange(ctx, duInst, envChanged, storeChanged, newStore, workList);
		}
	}
}

// Return true if there is a info flow violation
bool TaintAnalysis::runOnDefUseModule(const DefUseModule& duModule, bool reportError)
{
	auto workList = initializeWorkList(duModule);

	while (!workList.isEmpty())
	{
		const Context* ctx;
		const DefUseFunction* f;
		std::tie(ctx, f) = workList.dequeue();

		evalFunction(ctx, f, workList, duModule);		
	}

	//for (auto const& mapping: env)
	//	errs() << "\t" << ProgramLocation(mapping.first.first, mapping.first.second) << " -> " << (mapping.second == TaintLattice::Tainted) << "\n";

	return !transferFunction.checkMemoStates(env, memo, reportError);
}

}
}