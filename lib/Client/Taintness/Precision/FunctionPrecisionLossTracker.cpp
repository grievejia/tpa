#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/LocalDataFlowTracker.h"
#include "Client/Taintness/Precision/FunctionPrecisionLossTracker.h"
#include "Client/Taintness/Precision/PrecisionLossGlobalState.h"
#include "Client/Taintness/Precision/InterProcDataFlowTracker.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

FunctionPrecisionLossTracker::FunctionPrecisionLossTracker(const Context* c, const DefUseFunction* f, const TaintGlobalState& tg, PrecisionLossGlobalState& pg, GlobalWorkList& w): ctx(c), duFunc(f), globalState(tg), precGlobalState(pg), globalWorkList(w), localWorkList(globalWorkList.getLocalWorkList(ctx, duFunc)), transferFunction(globalState, ctx)
{
}

void FunctionPrecisionLossTracker::evalEntryInst(const DefUseInstruction* duInst)
{
	auto callers = globalState.getPointerAnalysis().getCallGraph().getCallSites(std::make_pair(ctx, &duFunc->getFunction()));

	std::vector<std::pair<const Context*, const Instruction*>> callerVec(callers.begin(), callers.end());
	CallDataFlowTracker(globalState, precGlobalState, globalWorkList).trackCall(DefUseProgramLocation(ctx, duInst), duFunc, callerVec);
}

void FunctionPrecisionLossTracker::evalExternalCallInst(const DefUseInstruction*, const Function*)
{
	// TODO: fill this in
	llvm_unreachable("Not implemented yet");
}

void FunctionPrecisionLossTracker::evalCallInst(const DefUseInstruction* duInst)
{
	auto callees = globalState.getPointerAnalysis().getCallGraph().getCallTargets(std::make_pair(ctx, duInst->getInstruction()));

	std::vector<std::pair<const Context*, const Function*>> nonExtCallees;
	for (auto callTgt: callees)
	{
		auto callee = callTgt.second;
		if (callee->isDeclaration())
			evalExternalCallInst(duInst, callee);
		else
			nonExtCallees.push_back(callTgt);
	}

	ReturnDataFlowTracker(globalState, precGlobalState, globalWorkList).trackReturn(DefUseProgramLocation(ctx, duInst), nonExtCallees);
}

void FunctionPrecisionLossTracker::evalInst(const DefUseInstruction* duInst, const TaintStore& store)
{
	auto inst = duInst->getInstruction();
	assert(inst != nullptr);

	switch (inst->getOpcode())
	{
		case Instruction::Alloca:
		case Instruction::Br:
			// These instructions never produce imprecise result
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
		case Instruction::PHI:
		{
			auto valueSet = transferFunction.evalAllOperands(duInst);
			LocalDataFlowTracker(localWorkList).trackValues(duInst, valueSet);
			break;
		}
		case Instruction::Store:
		{
			auto valueSet = transferFunction.evalStore(duInst);
			LocalDataFlowTracker(localWorkList).trackValues(duInst, valueSet);
			break;
		}
		case Instruction::Load:
		{
			auto memSet = transferFunction.evalLoad(duInst);
			LocalDataFlowTracker(localWorkList).trackMemory(duInst, memSet);
			break;
		}
		case Instruction::Invoke:
		case Instruction::Call:
		{
			evalCallInst(duInst);
			break;
		}
		case Instruction::Ret:
		{
			auto retVal = cast<ReturnInst>(inst)->getReturnValue();
			if (retVal != nullptr)
			{
				auto valueSet = transferFunction.evalAllOperands(duInst);
				LocalDataFlowTracker(localWorkList).trackValues(duInst, valueSet);
			}
			break;
		}
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
			errs() << *inst << "\n";
			llvm_unreachable("instruction not handled");
		}
	}
}

void FunctionPrecisionLossTracker::track()
{
	while (!localWorkList.isEmpty())
	{
		auto duInst = localWorkList.dequeue();

		if (!precGlobalState.insertVisitedLocation(DefUseProgramLocation(ctx, duInst)))
			continue;

		if (duInst->isEntryInstruction())
			evalEntryInst(duInst);
		else
		{
			errs() << "tracking " << *duInst->getInstruction() << "\n";
			auto optStore = globalState.getMemo().lookup(DefUseProgramLocation(ctx, duInst));
			auto const& store = (optStore == nullptr) ? TaintStore() : *optStore;
			evalInst(duInst, store);
		}
	}
}

}
}