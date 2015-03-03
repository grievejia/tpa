#include "Client/Taintness/TaintTransferFunction.h"
#include "MemoryModel/Memory/MemoryManager.h"
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

TaintTransferFunction::TaintTransferFunction(const PointerAnalysis& pa): ptrAnalysis(pa)
{
	ssManager.readSummaryFromFile("source_sink.conf");
}

std::tuple<bool, bool, bool> TaintTransferFunction::evalInst(const Context* ctx, const Instruction* inst, TaintState& state)
{
	auto checkTaint = [&state] (const Value* val)
	{
		if (isa<Constant>(val))
			return std::experimental::make_optional(TaintLattice::Untainted);
		return state.lookup(val);
	};

	auto checkTaintForAllOperands = [&checkTaint] (const Instruction* inst)
	{
		TaintLattice currVal = TaintLattice::Untainted;
		for (auto i = 0u, e = inst->getNumOperands(); i < e; ++i)
		{
			//errs() << "\tcheck op " << i << "\n";
			auto op = inst->getOperand(i);
			auto optVal = checkTaint(op);
			if (!optVal)
				return optVal;
			currVal = Lattice<TaintLattice>::merge(currVal, *optVal);
		}
		return std::experimental::make_optional(currVal);
	};

	switch (inst->getOpcode())
	{
		// Alloca
		case Instruction::Alloca:
		{
			auto allocInst = cast<AllocaInst>(inst);
			auto op0 = allocInst->getArraySize();

			auto optVal = state.lookup(op0);
			auto envChanged = state.strongUpdate(allocInst, optVal.value_or(TaintLattice::Untainted));

			return std::make_tuple(true, envChanged, false);
		}
		// Unary operators
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
		{
			auto optVal = checkTaintForAllOperands(inst);
			if (optVal)
			{
				auto envChanged = state.strongUpdate(inst, *optVal);
				return std::make_tuple(true, envChanged, false);
			}
			else
				return std::make_tuple(false, false, false);
		}
		case Instruction::PHI:
		{
			// Phi nodes has to proceed the analysis without full information
			auto optVal = checkTaintForAllOperands(inst);
			bool envChanged;
			if (optVal)
				envChanged = state.strongUpdate(inst, *optVal);
			else
				envChanged = state.strongUpdate(inst, TaintLattice::Untainted);
			return std::make_tuple(true, envChanged, false);
		}
		case Instruction::Store:
		{
			auto storeInst = cast<StoreInst>(inst);

			auto valOp = storeInst->getValueOperand();
			auto ptrOp = storeInst->getPointerOperand();

			auto optVal = checkTaint(valOp);
			if (!optVal)
				return std::make_tuple(false, false, false);

			auto ptsSet = ptrAnalysis.getPtsSet(ctx, ptrOp);
			assert(ptsSet != nullptr);
			auto needWeakUpdate = (ptsSet->getSize() > 1);
			bool storeChanged = false;
			if (!needWeakUpdate)
			{
				auto loc = *ptsSet->begin();
				if (!loc->isSummaryLocation())
					storeChanged = state.strongUpdate(loc, *optVal);
				else
					needWeakUpdate = true;
			}
			
			if (needWeakUpdate)
			{
				for (auto loc: *ptsSet)
					storeChanged |= state.weakUpdate(loc, *optVal);
			}

			return std::make_tuple(true, false, storeChanged);
		}
		case Instruction::Load:
		{
			auto loadInst = cast<LoadInst>(inst);

			auto ptrOp = loadInst->getPointerOperand();
			auto ptsSet = ptrAnalysis.getPtsSet(ctx, ptrOp);
			assert(ptsSet != nullptr);

			auto resVal = TaintLattice::Untainted;
			for (auto obj: *ptsSet)
			{
				auto optVal = state.lookup(obj);
				if (optVal)
					resVal = Lattice<TaintLattice>::merge(resVal, *optVal);
				else
				//	return std::make_tuple(false, false, false);
					resVal = TaintLattice::Untainted;
			}

			auto envChanged = state.strongUpdate(loadInst, resVal);
			return std::make_tuple(true, envChanged, false);
		}
		// TODO: Add implicit flow detection for Br
		case Instruction::Br:
			return std::make_tuple(true, false, false);
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
	llvm_unreachable("");
}

std::pair<bool, bool> TaintTransferFunction::processLibraryCall(const Context* ctx, const llvm::Function* callee, ImmutableCallSite cs, TaintState& state)
{
	auto envChanged = false;
	auto funName = callee->getName();
	auto ptrEffect = extTable.lookup(funName);

	if (!callee->getReturnType()->isVoidTy())
		envChanged |= state.strongUpdate(cs.getInstruction(), TaintLattice::Untainted);
	if (ptrEffect == PointerEffect::MemcpyArg1ToArg0 || funName == "strcpy" || funName == "strncpy")
	{
		auto dstSet = ptrAnalysis.getPtsSet(ctx, cs.getArgument(0));
		auto srcSet = ptrAnalysis.getPtsSet(ctx, cs.getArgument(1));
		assert(dstSet != nullptr && srcSet != nullptr);

		auto& memManager = ptrAnalysis.getMemoryManager();
		for (auto srcLoc: *srcSet)
		{
			auto srcLocs = memManager.getAllOffsetLocations(srcLoc);
			auto startingOffset = srcLoc->getOffset();
			for (auto oLoc: srcLocs)
			{
				auto optVal = state.lookup(oLoc);
				if (!optVal)
					continue;

				auto offset = oLoc->getOffset() - startingOffset;
				for (auto updateLoc: *dstSet)
				{
					auto tgtLoc = memManager.offsetMemory(updateLoc, offset);
					if (tgtLoc == memManager.getUniversalLocation())
						break;
					state.weakUpdate(tgtLoc, *optVal);
				}
			}
		}

		auto optVal = state.lookup(cs.getArgument(0));
		if (optVal)
			envChanged |= state.strongUpdate(cs.getInstruction(), *optVal);
	}
	else if (ptrEffect == PointerEffect::Malloc)
	{
		auto dstSet = ptrAnalysis.getPtsSet(ctx, cs.getInstruction());
		assert(dstSet != nullptr && dstSet->getSize() == 1);
		state.strongUpdate(*dstSet->begin(), TaintLattice::Untainted);
	}
	else if (auto summary = ssManager.getSummary(funName))
	{
		auto taintValue = [this, ctx, &state, &envChanged] (const TEntry& entry, const llvm::Value* val)
		{
			if (entry.what == TClass::ValueOnly)
				envChanged |= state.strongUpdate(val, entry.val);
			else if (entry.what == TClass::DirectMemory)
			{
				if (auto pSet = ptrAnalysis.getPtsSet(ctx, val))
				{
					for (auto loc: *pSet)
					{
						if (loc->isSummaryLocation())
							state.weakUpdate(loc, entry.val);
						else
							state.strongUpdate(loc, entry.val);
					}
				}
 			}
			else if (entry.what == TClass::ReachableMemory)
			{
				llvm_unreachable("not implemented yet");
			}
		};

		for (auto const& entry: *summary)
		{
			if (entry.end == TEnd::Sink)
				continue;
			switch (entry.pos)
			{
				case TPosition::Ret:
				{
					taintValue(entry, cs.getInstruction());
					break;
				}
				case TPosition::Arg0:
				{
					taintValue(entry, cs.getArgument(0));
					break;
				}
				case TPosition::Arg1:
				{
					taintValue(entry, cs.getArgument(1));
					break;
				}
				case TPosition::Arg2:
				{
					taintValue(entry, cs.getArgument(2));
					break;
				}
				case TPosition::Arg3:
				{
					taintValue(entry, cs.getArgument(3));
					break;
				}
				case TPosition::Arg4:
				{
					taintValue(entry, cs.getArgument(4));
					break;
				}
				case TPosition::AfterArg0:
				{
					for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
						taintValue(entry, cs.getArgument(i));

					break;
				}
				case TPosition::AfterArg1:
				{
					for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
						taintValue(entry, cs.getArgument(i));

					break;
				}
				case TPosition::AllArgs:
				{
					for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
						taintValue(entry, cs.getArgument(i));

					break;
				}
			}
		}
	}

	return std::make_pair(true, envChanged);
}

bool TaintTransferFunction::checkValue(const TEntry& entry, ProgramLocation pLoc, const Value* val, const TaintState& state)
{
	if (entry.what == TClass::ValueOnly)
	{
		auto sinkVal = state.lookup(val);
		if (sinkVal && Lattice<TaintLattice>::compare(*sinkVal, entry.val) == LatticeCompareResult::GreaterThan)
		{
			errs().changeColor(raw_ostream::Colors::RED);
			errs() << "Sink violation at " << *pLoc.getContext() << ":" << *pLoc.getInstruction() << "\n";
			errs().resetColor();
			return false;
		}
	}
	else if (entry.what == TClass::DirectMemory)
	{
		if (auto pSet = ptrAnalysis.getPtsSet(pLoc.getContext(), val))
		{
			for (auto loc: *pSet)
			{
				auto optVal = state.lookup(loc);
				if (optVal && Lattice<TaintLattice>::compare(*optVal, entry.val) == LatticeCompareResult::GreaterThan)
				{
					errs().changeColor(raw_ostream::Colors::RED);
					errs() << "Sink violation at " << *pLoc.getContext() << ":" << *pLoc.getInstruction() << "\n";
					errs().resetColor();
					return false;
				}
			}
		}
	}
	else if (entry.what == TClass::ReachableMemory)
	{
		llvm_unreachable("not implemented yet");
	}
	return true;
}

bool TaintTransferFunction::checkValue(const TSummary& summary, const Context* ctx, llvm::ImmutableCallSite cs, const TaintState& state)
{
	auto pLoc = ProgramLocation(ctx, cs.getInstruction());
	for (auto const& entry: summary)
	{
		if (entry.end == TEnd::Source)
			continue;

		switch (entry.pos)
		{
			case TPosition::Ret:
			{
				if (!checkValue(entry, pLoc, cs.getInstruction(), state))
					return false;
				break;
			}
			case TPosition::Arg0:
			{
				if (!checkValue(entry, pLoc, cs.getArgument(0), state))
					return false;
				break;
			}
			case TPosition::Arg1:
			{
				if (!checkValue(entry, pLoc, cs.getArgument(1), state))
					return false;
				break;
			}
			case TPosition::Arg2:
			{
				if (!checkValue(entry, pLoc, cs.getArgument(2), state))
					return false;
				break;
			}
			case TPosition::Arg3:
			{
				if (!checkValue(entry, pLoc, cs.getArgument(3), state))
					return false;
				break;
			}
			case TPosition::Arg4:
			{
				if (!checkValue(entry, pLoc, cs.getArgument(4), state))
					return false;
				break;
			}
			case TPosition::AfterArg0:
			{
				for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
					if (!checkValue(entry, pLoc, cs.getArgument(i), state))
						return false;
				break;
			}
			case TPosition::AfterArg1:
			{
				for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
					if (!checkValue(entry, pLoc, cs.getArgument(i), state))
						return false;
				break;
			}
			case TPosition::AllArgs:
			{
				for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
					if (!checkValue(entry, pLoc, cs.getArgument(i), state))
						return false;

				break;
			}
		}
	}

	return true;
}

bool TaintTransferFunction::checkMemoStates(const std::unordered_map<ProgramLocation, TaintState>& memo)
{
	for (auto const& mapping: memo)
	{
		auto const& pLoc = mapping.first;
		auto cs = ImmutableCallSite(pLoc.getInstruction());
		if (!cs)
			continue;

		auto callees = ptrAnalysis.getCallTargets(pLoc.getContext(), cs.getInstruction());
		for (auto callee: callees)
		{
			if (!callee->isDeclaration())
				continue;

			auto summary = ssManager.getSummary(callee->getName());
			if (summary == nullptr)
				continue;

			if (!checkValue(*summary, pLoc.getContext(), cs, mapping.second))
				return false;
		}
	}

	return true;
}

}
}
