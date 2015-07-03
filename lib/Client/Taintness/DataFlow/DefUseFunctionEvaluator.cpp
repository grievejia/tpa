#include "Client/Taintness/DataFlow/DefUseFunctionEvaluator.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
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

DefUseFunctionEvaluator::DefUseFunctionEvaluator(const Context* c, const DefUseFunction* f, TaintGlobalState& g, GlobalWorkListType& gl): ctx(c), duFunc(f), globalState(g), globalWorkList(gl), localWorkList(globalWorkList.getLocalWorkList(ctx, duFunc))
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
	
	for (auto const& mapping: duInst->mem_succs())
	{
		auto usedLoc = mapping.first;
		auto locVal = store.lookup(usedLoc);
		if (locVal == TaintLattice::Unknown)
			continue;
		for (auto succ: mapping.second)
		{
			if (memo.insert(DefUseProgramLocation(ctx, succ), usedLoc, locVal))
				workList.enqueue(succ);
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
	auto& oldLocalWorkList = globalWorkList.enqueueAndGetLocalWorkList(ctx, duFunc);

	propagateTopLevelChange(duInst, envChanged, oldLocalWorkList);
	propagateMemLevelChange(duInst, store, true, ctx, oldLocalWorkList);
}

void DefUseFunctionEvaluator::applyCall(const DefUseInstruction* duInst, const Context* newCtx, const Function* callee, const TaintStore& store)
{
	auto evalStatus = TaintTransferFunction(ctx, globalState).evalCallArguments(duInst->getInstruction(), newCtx, callee);
	if (evalStatus.isValid())
	{
		auto envChanged = evalStatus.hasEnvChanged();
		envChanged |= globalState.insertVisitedFunction(ProgramLocation(newCtx, callee));

		auto& duFunc = globalState.getProgram().getDefUseFunction(callee);
		auto entryDuInst = duFunc.getEntryInst();
		propagateGlobalState(newCtx, &duFunc, entryDuInst, store, envChanged);
	}
}

void DefUseFunctionEvaluator::applyExternalCall(const DefUseInstruction* duInst, const Function* callee, const TaintStore& store)
{
	auto newStore = store;
	auto evalStatus = TaintTransferFunction(ctx, newStore, globalState).evalExternalCall(duInst, callee);
	if (evalStatus.isValid())
		propagateState(duInst, newStore, evalStatus.hasEnvChanged(), evalStatus.hasStoreChanged());
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
			if (optRetVal != TaintLattice::Unknown)
				retVal = optRetVal;
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
	auto inst = duInst->getInstruction();
	EvalStatus evalStatus = EvalStatus::getInvalidStatus();
	TaintStore newStore;
	switch (inst->getOpcode())
	{
		case Instruction::Alloca:
			evalStatus = TaintTransferFunction(ctx, globalState).evalAlloca(inst);
			break;
		case Instruction::Trunc:
		case Instruction::ZExt:
		case Instruction::SExt:
		case Instruction::FPTrunc:
		case Instruction::FPExt:
		case Instruction::FPToUI:
		case Instruction::FPToSI:
		case Instruction::UIToFP:
		case Instruction::SIToFP:
		case Instruction::IntToPtr:
		case Instruction::PtrToInt:
		case Instruction::BitCast:
		case Instruction::AddrSpaceCast:
		case Instruction::ExtractElement:
		case Instruction::ExtractValue:
			// Binary operators
		case Instruction::And:
		case Instruction::Or:
		case Instruction::Xor:
		case Instruction::Shl:
		case Instruction::LShr:
		case Instruction::AShr:
		case Instruction::Add:
		case Instruction::FAdd:
		case Instruction::Sub:
		case Instruction::FSub:
		case Instruction::Mul:
		case Instruction::FMul:
		case Instruction::UDiv:
		case Instruction::SDiv:
		case Instruction::FDiv:
		case Instruction::URem:
		case Instruction::SRem:
		case Instruction::FRem:
		case Instruction::ICmp:
		case Instruction::FCmp:
		case Instruction::ShuffleVector:
		case Instruction::InsertElement:
		case Instruction::InsertValue:
			// Ternary operators
		case Instruction::Select:
			// N-ary operators
		case Instruction::GetElementPtr:
			evalStatus = TaintTransferFunction(ctx, globalState).evalAllOperands(inst);
			break;
		case Instruction::PHI:
			evalStatus = TaintTransferFunction(ctx, globalState).evalPhiNode(inst);
			break;
		case Instruction::Store:
			newStore = store;
			evalStatus = TaintTransferFunction(ctx, newStore, globalState).evalStore(inst);
			break;
		case Instruction::Load:
			newStore = store;
			evalStatus = TaintTransferFunction(ctx, newStore, globalState).evalLoad(inst);
			break;
		// TODO: Add implicit flow detection for Br
		case Instruction::Br:
			evalStatus = EvalStatus::getValidStatus(false, false);
			break;
		case Instruction::Invoke:
		case Instruction::Call:
		case Instruction::Ret:
		case Instruction::Switch:
		case Instruction::IndirectBr:
		case Instruction::AtomicCmpXchg:
		case Instruction::AtomicRMW:
		case Instruction::Fence:
		case Instruction::VAArg:
		case Instruction::LandingPad:
		case Instruction::Resume:
		case Instruction::Unreachable:
		{
			errs() << "Insruction not handled :" << *inst << "\n";
		}
	}
	if (evalStatus.isValid())
		propagateState(duInst, newStore, evalStatus.hasEnvChanged(), evalStatus.hasStoreChanged());
}

void DefUseFunctionEvaluator::evalEntry(const DefUseInstruction* duInst, const TaintStore& store)
{
	propagateState(duInst, store, true, true);
}

void DefUseFunctionEvaluator::eval()
{
	//errs() << "Context = " << *ctx << "\n";
	//errs() << "Function = " << duFunc->getFunction().getName() << "\n";

	while (!localWorkList.empty())
	{
		auto duInst = localWorkList.dequeue();

		//auto inst = duInst->getInstruction();
		//if (inst != nullptr)
		//	errs() << "inst = " << *inst << "\n";

		// Retrieve the corresponding store from memo
		auto optStore = globalState.getMemo().lookup(DefUseProgramLocation(ctx, duInst));
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