#include "Client/Taintness/TaintAnalysis.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Precision/KLimitContext.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

static const Instruction* getNextInstruction(const Instruction* inst)
{
	assert(!inst->isTerminator());
	auto itr = BasicBlock::const_iterator(inst);
	return ++itr;
}

ClientWorkList TaintAnalysis::initializeWorkList(const llvm::Module& module)
{
	auto worklist = ClientWorkList(module);

	// Find the entry function
	auto entryFunc = module.getFunction("main");
	if (entryFunc == nullptr)
		llvm_unreachable("Failed to find main function!");
	auto entryBlock = entryFunc->begin();

	// Set up the initial environment
	auto globalCtx = Context::getGlobalContext();
	auto initState = TaintState();

	if (entryFunc->arg_size() > 0)
	{
		auto argcValue = entryFunc->arg_begin();
		initState.strongUpdate(argcValue, TaintLattice::Tainted);
		if (entryFunc->arg_size() > 1)
		{
			auto argvValue = (++entryFunc->arg_begin());
			// TODO: is argv itself tainted or not? I would say no
			initState.strongUpdate(argvValue, TaintLattice::Untainted);

			auto& memManager = ptrAnalysis.getMemoryManager();
			auto argvPtrLoc = memManager.getArgvPtrLoc();
			auto argvMemLoc = memManager.getArgvMemLoc();
			initState.strongUpdate(argvPtrLoc, TaintLattice::Tainted);
			initState.strongUpdate(argvMemLoc, TaintLattice::Tainted);
		}
	}

	insertMemoState(ProgramLocation(globalCtx, entryBlock->begin()), std::move(initState));
	worklist.enqueue(globalCtx, entryFunc, entryBlock->begin());
	return worklist;
}

void TaintAnalysis::applyFunction(const Context* ctx, ImmutableCallSite cs, const Function* callee, const TaintState& state, ClientWorkList::LocalWorkList& workList, ClientWorkList& funWorkList)
{
	if (callee->isDeclaration())
	{
		if (callee->getName() == "exit" || callee->getName() == "_exit" || callee->getName() == "abort")
			return;
		
		auto newState = state;
		bool isValid;
		std::tie(isValid, std::ignore) = transferFunction.processLibraryCall(ctx, callee, cs, newState);
		if (isValid)
			propagateStateChange(ctx, cs.getInstruction(), newState, workList);
		return;
	}

	returnMap.insert(std::make_pair(callee, ProgramLocation(ctx, cs.getInstruction())));
	auto newCtx = KLimitContext::pushContext(ctx, cs.getInstruction(), callee);
	auto newState = state;

	assert(cs.arg_size() >= callee->arg_size());
	auto argNo = 0u;
	auto paramItr = callee->arg_begin();
	while (argNo < cs.arg_size() && paramItr != callee->arg_end())
	{
		auto arg = cs.getArgument(argNo);

		TaintLattice argVal = TaintLattice::Untainted;
		if (!isa<Constant>(arg))
		{
			auto optArgVal = state.lookup(arg);
			if (!optArgVal)
				return;
			argVal = *optArgVal;
		}

		newState.weakUpdate(paramItr, argVal);
		
		++argNo;
		++paramItr;
	}

	auto entryInst = callee->getEntryBlock().begin();
	insertMemoState(ProgramLocation(newCtx, entryInst), newState);

	funWorkList.enqueue(newCtx, callee, entryInst);
}

void TaintAnalysis::evalReturn(const tpa::Context* ctx, const llvm::Instruction* inst, const TaintState& state, ClientWorkList& funWorkList)
{
	auto retInst = cast<ReturnInst>(inst);

	auto range = returnMap.equal_range(inst->getParent()->getParent());
	assert(range.first != range.second && range.first != returnMap.end());

	bool hasReturnVal = false;
	auto retVal = TaintLattice::Untainted;
	if (auto ret = retInst->getReturnValue())
	{
		hasReturnVal = true;
		if (!isa<Constant>(ret))
		{
			auto optRetVal = state.lookup(ret);
			if (!optRetVal)
				return;
			retVal = *optRetVal;
		}
	}

	for (auto itr = range.first; itr != range.second; ++itr)
	{
		auto oldLoc = itr->second;
		auto oldCtx = oldLoc.getContext();
		auto oldInst = cast<Instruction>(oldLoc.getInstruction());
		auto oldFunc = oldInst->getParent()->getParent();
		auto& oldLocalWorkList = funWorkList.getLocalWorkList(oldCtx, oldFunc);
		funWorkList.enqueue(oldCtx, oldFunc);

		if (hasReturnVal)
		{
			auto newState = state;
			newState.weakUpdate(oldInst, retVal);
			propagateStateChange(oldCtx, oldInst, newState, oldLocalWorkList);
		}
		else
		{
			propagateStateChange(oldCtx, oldInst, state, oldLocalWorkList);
		}
	}
}

void TaintAnalysis::propagateStateChange(const Context* ctx, const Instruction* inst, const TaintState& state, ClientWorkList::LocalWorkList& workList)
{
	auto enqueueInst = [this, ctx, &state, &workList] (const Instruction* succInst)
	{
		if (insertMemoState(ProgramLocation(ctx, succInst), state))
		{
			workList.enqueue(succInst);
		}
	};

	if (auto termInst = dyn_cast<TerminatorInst>(inst))
	{
		assert(!isa<InvokeInst>(inst));
		for (auto i = 0u, e = termInst->getNumSuccessors(); i < e; ++i)
			enqueueInst(termInst->getSuccessor(i)->begin());
	}
	else
		enqueueInst(getNextInstruction(inst));
}

void TaintAnalysis::evalFunction(const Context* ctx, const Function* f, ClientWorkList& funWorkList, const Module& module)
{
	auto& workList = funWorkList.getLocalWorkList(ctx, f);

	while (!workList.isEmpty())
	{
		auto inst = workList.dequeue();

		errs() << "inst = " << *inst << "\n";
		auto itr = memo.find(ProgramLocation(ctx, inst));
		assert(itr != memo.end());
		auto const& state = itr->second;

		if (isa<CallInst>(inst) || isa<InvokeInst>(inst))
		{
			ImmutableCallSite cs(inst);
			assert(cs && "evalCall() gets an non-call?");

			auto calleeVec = std::vector<const Function*>();
			if (auto callee = cs.getCalledFunction())
				calleeVec.push_back(callee);
			else
				calleeVec = ptrAnalysis.getCallTargets(ctx, inst);

			bool mayCallNonExternal = false;
			for (auto callee: calleeVec)
			{
				if (!callee->isDeclaration())
					mayCallNonExternal = true;
				applyFunction(ctx, cs, callee, state, workList, funWorkList);
			}
			if (mayCallNonExternal)
				propagateStateChange(ctx, inst, state, workList);
		}
		else if (isa<ReturnInst>(inst))
		{
			if (inst->getParent()->getParent()->getName() == "main")
			{
				errs() << "T Reached Program end\n";
				continue;
			}

			evalReturn(ctx, inst, state, funWorkList);
		}
		else
		{
			bool isValid;
			auto newState = state;
			std::tie(isValid, std::ignore, std::ignore) = transferFunction.evalInst(ctx, inst, newState);
			//errs() << isValid << "\n";
			if (!isValid)
				continue;
			propagateStateChange(ctx, inst, newState, workList);
		}
	}
}

// Return true if there is a info flow violation
bool TaintAnalysis::runOnModule(const llvm::Module& module)
{
	auto workList = initializeWorkList(module);

	while (!workList.isEmpty())
	{
		const Context* ctx;
		const Function* f;
		std::tie(ctx, f) = workList.dequeue();

		evalFunction(ctx, f, workList, module);		
	}

	return !transferFunction.checkMemoStates(memo);
}

}
}