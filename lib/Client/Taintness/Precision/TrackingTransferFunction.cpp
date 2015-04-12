#include "MemoryModel/Memory/MemoryManager.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/TrackingTransferFunction.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TrackingTransferFunction::TrackingTransferFunction(const TaintGlobalState& g, const Context* c, ValueSet& v, MemorySet& m): globalState(g), ctx(c), valueSet(v), memSet(m)
{
}

TaintLattice TrackingTransferFunction::getTaintForValue(const Value* val)
{
	if (isa<Constant>(val))
		return TaintLattice::Untainted;
	return globalState.getEnv().lookup(ProgramLocation(ctx, val));
}

void TrackingTransferFunction::evalAllOperands(const Instruction* inst)
{
	auto rhsVal = getTaintForValue(inst);
	if (rhsVal != TaintLattice::Either)
		return;

	bool hasTaint = false, hasUntaint = false;
	std::vector<const Value*> candidateValues;
	candidateValues.reserve(inst->getNumOperands());
	for (auto i = 0u, e = inst->getNumOperands(); i < e; ++i)
	{
		auto op = inst->getOperand(i);
		auto opVal = getTaintForValue(op);
		switch (opVal)
		{
			case TaintLattice::Tainted:
				hasTaint = true;
				break;
			case TaintLattice::Untainted:
				hasUntaint = true;
				break;
			case TaintLattice::Either:
				candidateValues.push_back(op);
				break;
			case TaintLattice::Unknown:
				assert(false && "TrackingTransferFunction find Unknown operand?");
				break;
		}
	}

	// If we have both Taint and Untaint on the rhs, it's not useful to track any other operands even if they are imprecise, since the imprecision comes from path sensitivity
	if (hasTaint && hasUntaint)
		return;
	valueSet.insert(candidateValues.begin(), candidateValues.end());
}

template <typename SetType>
void TrackingTransferFunction::evalPtsSet(const Instruction* inst, const SetType& ptsSet)
{
	auto& memManager = globalState.getPointerAnalysis().getMemoryManager();
	auto store = globalState.getMemo().lookup(ProgramLocation(ctx, inst));
	assert(store != nullptr);

	bool hasTaint = false, hasUntaint = false;
	std::vector<const MemoryLocation*> candidateLocs;
	for (auto loc: ptsSet)
	{
		if (memManager.isSpecialMemoryLocation(loc))
			continue;

		auto locVal = store->lookup(loc);
		switch (locVal)
		{
			case TaintLattice::Tainted:
				hasTaint = true;
				break;
			case TaintLattice::Untainted:
				hasUntaint = true;
				break;
			case TaintLattice::Either:
				candidateLocs.push_back(loc);
				break;
			case TaintLattice::Unknown:
				assert(false && "TrackingTransferFunction find Unknown operand?");
				break;
		}
	}

	// If we have both Taint and Untaint on the rhs, it's not useful to track any other operands even if they are  imprecise, since the imprecision comes from path sensitivity
	if (hasTaint && hasUntaint)
		return;
	memSet.insert(candidateLocs.begin(), candidateLocs.end());
}

void TrackingTransferFunction::evalLoad(const Instruction* inst)
{
	auto loadInst = cast<LoadInst>(inst);

	auto ptrOp = loadInst->getPointerOperand();
	auto ptsSet = globalState.getPointerAnalysis().getPtsSet(ctx, ptrOp);
	assert(!ptsSet.isEmpty());

	evalPtsSet(inst, ptsSet);
}

void TrackingTransferFunction::evalStore(const Instruction* inst)
{
	auto storeInst = cast<StoreInst>(inst);

	auto valOp = storeInst->getValueOperand();
	auto storeVal = getTaintForValue(valOp);
	switch (storeVal)
	{
		case TaintLattice::Tainted:
		case TaintLattice::Untainted:
			// If the value is precise and this store inst is still enqueued, it means that the imprecision comes from weak update, about which we cannot do anything
			break;
		case TaintLattice::Either:
			valueSet.insert(valOp);
			break;
		case TaintLattice::Unknown:
			assert(false && "TrackingTransferFunction find Unknown operand?");
			break;
	}
}

void TrackingTransferFunction::evalMemcpy(const DefUseInstruction* duInst)
{
	auto inst = duInst->getInstruction();
	ImmutableCallSite cs(inst);

	auto const& ptrAnalysis = globalState.getPointerAnalysis();
	auto& memManager = ptrAnalysis.getMemoryManager();
	auto srcSet = ptrAnalysis.getPtsSet(ctx, cs.getArgument(1));
	assert(!srcSet.isEmpty());

	// Categorizing the source locations based on their offset
	std::unordered_map<size_t, MemorySet> offsetMap;
	for (auto srcLoc: srcSet)
	{
		auto srcLocs = memManager.getAllOffsetLocations(srcLoc);
		auto startingOffset = srcLoc->getOffset();
		for (auto oLoc: srcLocs)
		{
			if (memManager.isSpecialMemoryLocation(oLoc))
				continue;
			auto offset = oLoc->getOffset() - startingOffset;
			offsetMap[offset].insert(oLoc);
		}
	}

	for (auto const& mapping: offsetMap)
		evalPtsSet(inst, mapping.second);
}

void TrackingTransferFunction::evalExternalCall(const DefUseInstruction* duInst, const Function* callee)
{
	auto funName = callee->getName();

	auto ptrEffect = globalState.getExternalPointerEffectTable().lookup(funName);
	if (ptrEffect == PointerEffect::MemcpyArg1ToArg0 || funName == "strcpy" || funName == "strncpy")
		evalMemcpy(duInst);
}

void TrackingTransferFunction::evalNonExternalCall(const DefUseInstruction* duInst)
{
	// We don't actually know what's going on with the callees. Therefore, conservatively assume all imprecise sources are causes of the callinst's imprecision and try to track'em all
	ImmutableCallSite cs(duInst->getInstruction());
	for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
	{
		auto arg = cs.getArgument(i);
		if (getTaintForValue(arg) == TaintLattice::Either)
			valueSet.insert(arg);
	}

	auto optStore = globalState.getMemo().lookup(ProgramLocation(ctx, duInst->getInstruction()));
	auto const& store = (optStore == nullptr) ? TaintStore() : *optStore;
	auto const& memManager = globalState.getPointerAnalysis().getMemoryManager();
	for (auto const& mapping: duInst->mem_preds())
	{
		auto loc = mapping.first;
		if (memManager.isSpecialMemoryLocation(loc))
			continue;

		auto locVal = store.lookup(loc);
		if (locVal == TaintLattice::Either)
			memSet.insert(loc);
	}
}

void TrackingTransferFunction::evalCall(const DefUseInstruction* duInst)
{
	auto callees = globalState.getPointerAnalysis().getCallGraph().getCallTargets(std::make_pair(ctx, duInst->getInstruction()));
	for (auto callTgt: callees)
	{
		auto callee = callTgt.second;
		if (callee->isDeclaration())
			evalExternalCall(duInst, callee);
		else
			evalNonExternalCall(duInst);
	}
}

void TrackingTransferFunction::eval(const DefUseInstruction* duInst)
{
	if (duInst->isEntryInstruction())
		return;

	auto inst = duInst->getInstruction();
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
			evalAllOperands(inst);
			break;
		case Instruction::Store:
			evalStore(inst);
			break;
		case Instruction::Load:
			evalLoad(inst);
			break;
		case Instruction::Invoke:
		case Instruction::Call:
			evalCall(duInst);
			break;
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
			errs() << *inst << "\n";
			llvm_unreachable("instruction not handled");
		}
	}
}

// Explicit template instantiation to avoid linking error
template void TrackingTransferFunction::evalPtsSet<PtsSet>(const Instruction*, const PtsSet&);
template void TrackingTransferFunction::evalPtsSet<TrackingTransferFunction::MemorySet>(const Instruction*, const MemorySet&);

}
}