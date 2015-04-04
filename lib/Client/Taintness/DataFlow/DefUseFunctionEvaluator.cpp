#include "Client/Taintness/DataFlow/DefUseFunctionEvaluator.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/DataFlow/TaintMemo.h"
#include "Client/Taintness/DataFlow/TaintTransferFunction.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

DefUseFunctionEvaluator::DefUseFunctionEvaluator(const Context* c, const DefUseFunction* f, TaintGlobalState& g, GlobalWorkListType& gl, TaintTransferFunction& t): ctx(c), duFunc(f), globalState(g), globalWorkList(gl), localWorkList(globalWorkList.getLocalWorkList(ctx, duFunc)), transferFunction(t)
{
}

void DefUseFunctionEvaluator::propagateTopLevelChange(const DefUseInstruction* duInst, bool envChanged, LocalWorkListType& workList)
{
	if (envChanged)
	{
		for (auto succ: duInst->top_succs())
			workList.enqueue(succ);
	}
}

void DefUseFunctionEvaluator::propagateMemLevelChange(const DefUseInstruction* duInst, const TaintStore& store, bool storeChanged, const Context* ctx, LocalWorkListType& workList)
{
	auto& memo = globalState.getMemo();
	if (storeChanged)
	{
		for (auto const& mapping: duInst->mem_succs())
		{
			auto usedLoc = mapping.first;
			auto optLocVal = store.lookup(usedLoc);
			if (!optLocVal)
				continue;
			
			for (auto succ: mapping.second)
			{
				if (memo.insert(ProgramLocation(ctx, succ->getInstruction()), usedLoc, *optLocVal))
					workList.enqueue(succ);
			}
		}
	}
}

void DefUseFunctionEvaluator::propagateState(const DefUseInstruction* duInst, const TaintStore& store, bool envChanged, bool storeChanged)
{
	propagateTopLevelChange(duInst, envChanged, localWorkList);
	propagateMemLevelChange(duInst, store, storeChanged, ctx, localWorkList);
}

void DefUseFunctionEvaluator::propagateGlobalState(const Context* ctx, const DefUseFunction* duFunc, const DefUseInstruction* duInst, const TaintStore& store, bool envChanged)
{
	globalWorkList.enqueue(ctx, duFunc);
	auto& oldLocalWorkList = globalWorkList.getLocalWorkList(ctx, duFunc);

	propagateTopLevelChange(duInst, envChanged, oldLocalWorkList);
	propagateMemLevelChange(duInst, store, true, ctx, oldLocalWorkList);
}

std::vector<TaintLattice> DefUseFunctionEvaluator::collectArgumentTaintValue(ImmutableCallSite cs, const Function* callee)
{
	std::vector<TaintLattice> callerVals;
	callerVals.reserve(callee->arg_size());
	for (auto i = 0ul, e = callee->arg_size(); i < e; ++i)
	{
		auto arg = cs.getArgument(i);
		auto argVal = TaintLattice::Untainted;
		if (!isa<Constant>(arg))
		{
			auto optArgVal = globalState.getEnv().lookup(ProgramLocation(ctx, arg));
			if (!optArgVal)
				break;
			argVal = *optArgVal;
		}

		callerVals.push_back(argVal);
	}
	return callerVals;
}

bool DefUseFunctionEvaluator::updateParamTaintValue(const Context* newCtx, const Function* callee, const std::vector<TaintLattice>& argVals)
{
	auto ret = false;
	auto paramItr = callee->arg_begin();
	for (auto argVal: argVals)
	{
		ret |= globalState.getEnv().weakUpdate(ProgramLocation(newCtx, paramItr), argVal);
		++paramItr;
	}
	return ret;
}

void DefUseFunctionEvaluator::applyCall(const DefUseInstruction* duInst, const Context* newCtx, const Function* callee, const TaintStore& store)
{
	ImmutableCallSite cs(duInst->getInstruction());
	assert(cs);
	assert(cs.arg_size() >= callee->arg_size());

	auto argVals = collectArgumentTaintValue(cs, callee);
	if (argVals.size() < callee->arg_size())
		return;
	bool envChanged = false;
	envChanged |= updateParamTaintValue(newCtx, callee, argVals);
	envChanged |= globalState.insertVisitedFunction(ProgramLocation(newCtx, callee));

	auto& duFunc = globalState.getProgram().getDefUseFunction(callee);
	auto entryDuInst = duFunc.getEntryInst();
	propagateGlobalState(newCtx, &duFunc, entryDuInst, store, envChanged);
}

void DefUseFunctionEvaluator::applyExternalCall(const DefUseInstruction* duInst, const Function* callee, const TaintStore& store)
{
	ImmutableCallSite cs(duInst->getInstruction());
	assert(cs);

	bool isValid, envChanged, storeChanged;
	auto newStore = store;
	std::tie(isValid, envChanged, storeChanged) = transferFunction.processLibraryCall(ctx, callee, cs, globalState.getEnv(), newStore);
	if (!isValid)
		return;
	propagateState(duInst, newStore, envChanged, storeChanged);
}

void DefUseFunctionEvaluator::evalCall(const DefUseInstruction* duInst, const TaintStore& store)
{
	auto inst = duInst->getInstruction();

	auto callees = globalState.getPointerAnalysis().getCallGraph().getCallTargets(std::make_pair(ctx, inst));
	for (auto callTgt: callees)
	{
		auto newCtx = callTgt.first;
		auto callee = callTgt.second;
		if (callee->isDeclaration())
			applyExternalCall(duInst, callee, store);
		else
			applyCall(duInst, newCtx, callee, store);
	}
}

std::experimental::optional<TaintLattice> DefUseFunctionEvaluator::getReturnTaintValue(const DefUseInstruction* duInst)
{
	auto retInst = cast<ReturnInst>(duInst->getInstruction());
	std::experimental::optional<TaintLattice> retVal;

	if (auto ret = retInst->getReturnValue())
	{
		if (isa<Constant>(ret))
			retVal = TaintLattice::Untainted;
		else
		{
			auto optRetVal = globalState.getEnv().lookup(ProgramLocation(ctx, ret));
			if (optRetVal)
				retVal = *optRetVal;
		}
	}
	return retVal;
}

void DefUseFunctionEvaluator::applyReturn(const Context* oldCtx, const Instruction* oldInst, std::experimental::optional<TaintLattice> retVal, const TaintStore& store)
{
	auto oldFunc = oldInst->getParent()->getParent();
	auto& oldDuFunc = globalState.getProgram().getDefUseFunction(oldFunc);
	auto oldDuInst = oldDuFunc.getDefUseInstruction(oldInst);

	auto envChanged = false;
	if (retVal)
		envChanged |= globalState.getEnv().weakUpdate(ProgramLocation(oldCtx, oldInst), *retVal);

	propagateGlobalState(oldCtx, &oldDuFunc, oldDuInst, store, envChanged);
}

void DefUseFunctionEvaluator::evalReturn(const DefUseInstruction* duInst, const TaintStore& store)
{
	auto fromFunc = duInst->getFunction();
	if (fromFunc->getName() == "main")
	{
		errs() << "T Reached Program end\n";
		return;
	}

	auto optRetVal = getReturnTaintValue(duInst);

	auto returnTgts = globalState.getPointerAnalysis().getCallGraph().getCallSites(std::make_pair(ctx, fromFunc));
	for (auto retTgt: returnTgts)
	{
		auto oldCtx = retTgt.first;
		auto oldInst = retTgt.second;

		applyReturn(oldCtx, oldInst, optRetVal, store);
	}
}

void DefUseFunctionEvaluator::evalInst(const DefUseInstruction* duInst, const TaintStore& store)
{
	bool isValid, envChanged, storeChanged;
	auto newStore = store;
	std::tie(isValid, envChanged, storeChanged) = transferFunction.evalInst(ctx, duInst->getInstruction(), globalState.getEnv(), newStore);
	if (!isValid)
		return;
	propagateState(duInst, newStore, envChanged, storeChanged);
}

void DefUseFunctionEvaluator::evalEntry(const DefUseInstruction* duInst, const TaintStore& store)
{
	propagateState(duInst, store, true, true);
}

void DefUseFunctionEvaluator::eval()
{
	//errs() << "Context = " << *ctx << "\n";
	//errs() << "Function = " << duFunc->getFunction().getName() << "\n";

	while (!localWorkList.isEmpty())
	{
		auto duInst = localWorkList.dequeue();

		//auto inst = duInst->getInstruction();
		//if (inst != nullptr)
		//	errs() << "inst = " << *inst << "\n";

		// Retrieve the corresponding store from memo
		auto optStore = globalState.getMemo().lookup(ProgramLocation(ctx, duInst->getInstruction()));
		auto const& store = (optStore == nullptr) ? TaintStore() : *optStore;

		if (duInst->isEntryInstruction())
			evalEntry(duInst, store);
		else if (duInst->isCallInstruction())
			evalCall(duInst, store);
		else if (duInst->isReturnInstruction())
			evalReturn(duInst, store);
		else
			evalInst(duInst, store);
	}
}

}
}