#include "Context/KLimitContext.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/Support/FunctionContext.h"
#include "TaintAnalysis/Engine/TaintGlobalState.h"
#include "TaintAnalysis/Engine/TransferFunction.h"
#include "TaintAnalysis/Program/DefUseModule.h"
#include "TaintAnalysis/Support/ProgramPoint.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "Util/IO/TaintAnalysis/Printer.h"

#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;
using namespace util::io;

namespace taint
{

void TransferFunction::addTopLevelSuccessors(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto duInst = pp.getDefUseInstruction();
	for (auto succ: duInst->top_succs())
		evalResult.addTopLevelSuccessor(ProgramPoint(ctx, succ));
}

void TransferFunction::addMemLevelSuccessors(const ProgramPoint& pp, const tpa::MemoryObject* obj, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto duInst = pp.getDefUseInstruction();
	for (auto succ: duInst->mem_succs(obj))
		evalResult.addMemLevelSuccessor(ProgramPoint(ctx, succ), obj);
}

void TransferFunction::addMemLevelSuccessors(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto duInst = pp.getDefUseInstruction();
	for (auto const& mapping: duInst->mem_succs())
		for (auto succ: mapping.second)
			evalResult.addMemLevelSuccessor(ProgramPoint(ctx, succ), mapping.first);
}

TaintLattice TransferFunction::getTaintForOperands(const context::Context* ctx, const Instruction* inst)
{
	TaintLattice currVal = TaintLattice::Unknown;
	auto& env = globalState.getEnv();
	for (auto i = 0u, e = inst->getNumOperands(); i < e; ++i)
	{
		auto op = inst->getOperand(i);
		auto opVal = env.lookup(TaintValue(ctx, op));
		currVal = Lattice<TaintLattice>::merge(currVal, opVal);
	}
	return currVal;
}

void TransferFunction::evalEntry(const ProgramPoint& pp, EvalResult& evalResult, bool envChanged)
{
	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
	addMemLevelSuccessors(pp, evalResult);
}

void TransferFunction::evalAlloca(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto duInst = pp.getDefUseInstruction();

	auto envChanged = globalState.getEnv().strongUpdate(TaintValue(pp.getContext(), duInst->getInstruction()), TaintLattice::Untainted);

	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
}

void TransferFunction::evalAllOperands(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto tVal = getTaintForOperands(pp.getContext(), pp.getDefUseInstruction()->getInstruction());

	if (tVal != TaintLattice::Unknown)
	{
		auto envChanged = globalState.getEnv().strongUpdate(TaintValue(pp.getContext(), pp.getDefUseInstruction()->getInstruction()), tVal);
		if (envChanged)
			addTopLevelSuccessors(pp, evalResult);
	}
}

TaintLattice TransferFunction::loadTaintFromPtsSet(tpa::PtsSet pSet, const TaintStore& store)
{
	TaintLattice resVal = TaintLattice::Unknown;
	for (auto obj: pSet)
	{
		if (obj->isUniversalObject())
		{
			resVal = TaintLattice::Either;
			break;
		}
		else if (obj->isNullObject())
			continue;

		auto locVal = store.lookup(obj);
		resVal = Lattice<TaintLattice>::merge(resVal, locVal);
	}

	return resVal;
}

void TransferFunction::evalLoad(const ProgramPoint& pp, EvalResult& evalResult)
{
	if (localState == nullptr)
		return;

	auto ctx = pp.getContext();
	auto loadInst = cast<LoadInst>(pp.getDefUseInstruction()->getInstruction());

	auto ptrOp = loadInst->getPointerOperand();
	auto ptsSet = globalState.getPointerAnalysis().getPtsSet(ctx, ptrOp);
	assert(!ptsSet.empty());

	auto loadVal = loadTaintFromPtsSet(ptsSet, *localState);
	if (loadVal != TaintLattice::Unknown)
	{
		auto envChanged = globalState.getEnv().strongUpdate(TaintValue(pp.getContext(), pp.getDefUseInstruction()->getInstruction()), loadVal);
		if (envChanged)
			addTopLevelSuccessors(pp, evalResult);
	}
}

void TransferFunction::strongUpdateStore(const MemoryObject* obj, TaintLattice v, TaintStore& store)
{
	store.strongUpdate(obj, v);
}

void TransferFunction::weakUpdateStore(PtsSet pSet, TaintLattice v, TaintStore& store)
{
	for (auto obj: pSet)
	{
		if (obj->isSpecialObject())
			continue;
		store.weakUpdate(obj, v);
	}
}

void TransferFunction::evalStore(const ProgramPoint& pp, EvalResult& evalResult)
{
	if (localState != nullptr)
		evalResult.setStore(*localState);

	auto ctx = pp.getContext();
	auto storeInst = cast<StoreInst>(pp.getDefUseInstruction()->getInstruction());

	auto valOp = storeInst->getValueOperand();
	auto ptrOp = storeInst->getPointerOperand();

	auto val = globalState.getEnv().lookup(TaintValue(ctx, valOp));
	if (val == TaintLattice::Unknown)
		return;

	auto ptsSet = globalState.getPointerAnalysis().getPtsSet(ctx, ptrOp);
	assert(!ptsSet.empty());
	auto obj = *ptsSet.begin();
	
	if (ptsSet.size() == 1 && !obj->isSummaryObject())
		strongUpdateStore(obj, val, evalResult.getStore());
	else
		weakUpdateStore(ptsSet, val, evalResult.getStore());

	for (auto obj: ptsSet)
		addMemLevelSuccessors(pp, obj, evalResult);
}

std::vector<TaintLattice> TransferFunction::collectArgumentTaintValue(const context::Context* ctx, const ImmutableCallSite& cs, size_t numParam)
{
	std::vector<TaintLattice> callerVals;
	callerVals.reserve(numParam);

	auto const& env = globalState.getEnv();
	for (auto i = 0ul, e = numParam; i < e; ++i)
	{
		auto arg = cs.getArgument(i);
		auto argVal = env.lookup(TaintValue(ctx, arg));
		if (argVal == TaintLattice::Unknown)
			break;
		callerVals.push_back(argVal);
	}
	return callerVals;
}

bool TransferFunction::updateParamTaintValue(const context::Context* newCtx, const Function* callee, const std::vector<TaintLattice>& argVals)
{
	auto ret = false;
	auto paramItr = callee->arg_begin();
	for (auto argVal: argVals)
	{
		ret |= globalState.getEnv().weakUpdate(TaintValue(newCtx, paramItr), argVal);
		++paramItr;
	}
	return ret;
}

void TransferFunction::evalInternalCall(const ProgramPoint& pp, const tpa::FunctionContext& fc, EvalResult& evalResult, bool callGraphUpdated)
{
	ImmutableCallSite cs(pp.getDefUseInstruction()->getInstruction());
	assert(cs);

	auto callee = fc.getFunction();
	auto numParam = callee->arg_size();
	assert(cs.arg_size() >= numParam);

	auto argSets = collectArgumentTaintValue(pp.getContext(), cs, numParam);
	if (argSets.size() < numParam)
		return;

	auto envChanged = updateParamTaintValue(fc.getContext(), callee, argSets) || callGraphUpdated;
	auto entryInst = globalState.getDefUseModule().getDefUseFunction(fc.getFunction()).getEntryInst();

	evalEntry(ProgramPoint(fc.getContext(), entryInst), evalResult, envChanged);
}

void TransferFunction::evalCall(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto inst = pp.getDefUseInstruction()->getInstruction();
	ImmutableCallSite cs(inst);
	assert(cs);

	if (localState != nullptr)
		evalResult.setStore(*localState);

	auto ctx = pp.getContext();
	auto callees = globalState.getPointerAnalysis().getCallees(cs, ctx);
	for (auto callTgt: callees)
	{
		if (callTgt->isDeclaration())
			evalExternalCall(pp, callTgt, evalResult);
		else
		{
			auto newCtx = context::KLimitContext::pushContext(ctx, inst);
			auto fc = FunctionContext(newCtx, callTgt);
			auto callGraphUpdated = globalState.getCallGraph().insertEdge(pp, fc);

			evalInternalCall(pp, fc, evalResult, callGraphUpdated);
		}
	}
}

void TransferFunction::applyReturn(const ProgramPoint& pp, TaintLattice tVal, EvalResult& evalResult)
{
	auto envChanged = false;
	if (tVal != TaintLattice::Unknown)
		envChanged = globalState.getEnv().weakUpdate(TaintValue(pp.getContext(), pp.getDefUseInstruction()->getInstruction()), tVal);

	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
	addMemLevelSuccessors(pp, evalResult);
}

void TransferFunction::evalReturn(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto duInst = pp.getDefUseInstruction();
	auto retInst = cast<ReturnInst>(duInst->getInstruction());

	auto fromFunc = duInst->getFunction();
	if (fromFunc->getName() == "main")
	{
		errs() << "T Reached Program end\n";
		return;
	}

	if (localState != nullptr)
		evalResult.setStore(*localState);

	auto tVal = TaintLattice::Unknown;
	if (auto retVal = retInst->getReturnValue())
		tVal = globalState.getEnv().lookup(TaintValue(ctx, retVal));

	auto returnTgts = globalState.getCallGraph().getCallers(FunctionContext(ctx, fromFunc));
	for (auto const& retSite: returnTgts)
		applyReturn(retSite, tVal, evalResult);
}


EvalResult TransferFunction::eval(const ProgramPoint& pp)
{
	EvalResult evalResult;

	//errs() << "eval " << pp << "\n";

	auto duInst = pp.getDefUseInstruction();
	if (duInst->isEntryInstruction())
	{
		evalResult.setStore(*localState);
		evalEntry(pp, evalResult, true);
	}
	else
	{
		auto inst = duInst->getInstruction();
		assert(inst != nullptr);

		switch (inst->getOpcode())
		{
			case Instruction::Alloca:
				evalAlloca(pp, evalResult);
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
				evalAllOperands(pp, evalResult);
				break;
			case Instruction::Store:
				evalStore(pp, evalResult);
				break;
			case Instruction::Load:
				evalLoad(pp, evalResult);
				break;
			// TODO: Add implicit flow detection for Br
			case Instruction::Br:
				break;
			case Instruction::Invoke:
			case Instruction::Call:
				evalCall(pp, evalResult);
				break;
			case Instruction::Ret:
				evalReturn(pp, evalResult);
				break;
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
				llvm_unreachable("Taint analysis failed");
			}
		}
	}

	return evalResult;
}

}
