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

TaintTransferFunction::TaintTransferFunction(const PointerAnalysis& pa, const tpa::ExternalPointerEffectTable& e): ptrAnalysis(pa), extTable(e)
{
	ssManager.readSummaryFromFile("source_sink.conf");
	uLoc = ptrAnalysis.getMemoryManager().getUniversalLocation();
	nLoc = ptrAnalysis.getMemoryManager().getNullLocation();
}

std::tuple<bool, bool, bool> TaintTransferFunction::evalInst(const Context* ctx, const Instruction* inst, TaintEnv& env, TaintStore& store)
{
	auto checkTaint = [ctx, &env] (const Value* val)
	{
		if (isa<Constant>(val))
			return std::experimental::make_optional(TaintLattice::Untainted);
		return env.lookup(ProgramLocation(ctx, val));
	};

	auto checkTaintForAllOperands = [&checkTaint] (const Instruction* inst)
	{
		TaintLattice currVal = TaintLattice::Untainted;
		for (auto i = 0u, e = inst->getNumOperands(); i < e; ++i)
		{
			auto op = inst->getOperand(i);
			//errs() << "\tcheck op " << i << ": " << *op << "\n";
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

			auto optVal = env.lookup(ProgramLocation(ctx, op0));
			auto envChanged = env.strongUpdate(ProgramLocation(ctx, allocInst), optVal.value_or(TaintLattice::Untainted));

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
				auto envChanged = env.strongUpdate(ProgramLocation(ctx, inst), *optVal);
				return std::make_tuple(true, envChanged, false);
			}
			else
				return std::make_tuple(false, false, false);
		}
		case Instruction::PHI:
		{
			// Phi nodes has to make progress without all operands information available
			auto optVal = checkTaintForAllOperands(inst);
			bool envChanged;
			if (optVal)
				envChanged = env.strongUpdate(ProgramLocation(ctx, inst), *optVal);
			else
				envChanged = env.strongUpdate(ProgramLocation(ctx, inst), TaintLattice::Untainted);
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
					storeChanged = store.strongUpdate(loc, *optVal);
				else
					needWeakUpdate = true;
			}
			
			if (needWeakUpdate)
			{
				for (auto loc: *ptsSet)
				{
					if (loc == uLoc || loc == nLoc)
						continue;
					storeChanged |= store.weakUpdate(loc, *optVal);
				}
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
				if (obj == uLoc)
				{
					resVal = TaintLattice::Tainted;
					break;
				}
				else if (obj == nLoc)
				{
					continue;
				}

				auto optVal = store.lookup(obj);
				if (optVal)
					resVal = Lattice<TaintLattice>::merge(resVal, *optVal);
				else
					return std::make_tuple(false, false, false);
				//	resVal = TaintLattice::Untainted;
			}

			auto envChanged = env.strongUpdate(ProgramLocation(ctx, loadInst), resVal);
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

std::tuple<bool, bool, bool> TaintTransferFunction::processLibraryCall(const Context* ctx, const llvm::Function* callee, ImmutableCallSite cs, TaintEnv& env, TaintStore& store)
{
	auto envChanged = false, storeChanged = false;
	auto funName = callee->getName();
	auto ptrEffect = extTable.lookup(funName);

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
				auto optVal = store.lookup(oLoc);
				if (!optVal)
					continue;

				auto offset = oLoc->getOffset() - startingOffset;
				for (auto updateLoc: *dstSet)
				{
					auto tgtLoc = memManager.offsetMemory(updateLoc, offset);
					if (tgtLoc == uLoc)
						break;
					storeChanged |= store.weakUpdate(tgtLoc, *optVal);
				}
			}
		}

		auto optVal = env.lookup(ProgramLocation(ctx, cs.getArgument(0)));
		if (optVal)
			envChanged |= env.strongUpdate(ProgramLocation(ctx, cs.getInstruction()), *optVal);
	}
	else if (ptrEffect == PointerEffect::Malloc)
	{
		auto dstSet = ptrAnalysis.getPtsSet(ctx, cs.getInstruction());
		assert(dstSet != nullptr && dstSet->getSize() == 1);
		storeChanged |= store.strongUpdate(*dstSet->begin(), TaintLattice::Untainted);
		envChanged |= env.strongUpdate(ProgramLocation(ctx, cs.getInstruction()), TaintLattice::Untainted);
	}
	if (auto summary = ssManager.getSummary(funName))
	{
		auto taintValue = [this, ctx, &env, &store, &envChanged, &storeChanged] (const TEntry& entry, const llvm::Value* val)
		{
			if (entry.what == TClass::ValueOnly)
				envChanged |= env.strongUpdate(ProgramLocation(ctx, val), entry.val);
			else if (entry.what == TClass::DirectMemory)
			{
				if (auto pSet = ptrAnalysis.getPtsSet(ctx, val))
				{
					for (auto loc: *pSet)
					{
						if (loc->isSummaryLocation())
							storeChanged |= store.weakUpdate(loc, entry.val);
						else
							storeChanged |= store.strongUpdate(loc, entry.val);
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
			{
				sinkPoints.insert({ctx, cs.getInstruction(), callee});
				continue;
			}
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
	else if (!callee->getReturnType()->isVoidTy())
		envChanged |= env.weakUpdate(ProgramLocation(ctx, cs.getInstruction()), TaintLattice::Untainted);

	return std::make_tuple(true, envChanged, storeChanged);
}

bool TaintTransferFunction::checkValue(const TEntry& entry, ProgramLocation pLoc, const TaintEnv& env, const TaintStore& store)
{
	if (entry.what == TClass::ValueOnly)
	{
		auto sinkVal = env.lookup(pLoc);
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
		if (auto pSet = ptrAnalysis.getPtsSet(pLoc.getContext(), pLoc.getInstruction()))
		{
			for (auto loc: *pSet)
			{
				auto optVal = store.lookup(loc);
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

bool TaintTransferFunction::checkValue(const TSummary& summary, const Context* ctx, llvm::ImmutableCallSite cs, const TaintEnv& env, const TaintStore& store)
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
				if (!checkValue(entry, pLoc, env, store))
					return false;
				break;
			}
			case TPosition::Arg0:
			{
				if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(0)), env, store))
					return false;
				break;
			}
			case TPosition::Arg1:
			{
				if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(1)), env, store))
					return false;
				break;
			}
			case TPosition::Arg2:
			{
				if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(2)), env, store))
					return false;
				break;
			}
			case TPosition::Arg3:
			{
				if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(3)), env, store))
					return false;
				break;
			}
			case TPosition::Arg4:
			{
				if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(4)), env, store))
					return false;
				break;
			}
			case TPosition::AfterArg0:
			{
				for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
					if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(i)), env, store))
						return false;
				break;
			}
			case TPosition::AfterArg1:
			{
				for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
					if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(i)), env, store))
						return false;
				break;
			}
			case TPosition::AllArgs:
			{
				for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
					if (!checkValue(entry, ProgramLocation(ctx, cs.getArgument(i)), env, store))
						return false;

				break;
			}
		}
	}

	return true;
}

bool TaintTransferFunction::checkMemoStates(const TaintEnv& env, const std::unordered_map<tpa::ProgramLocation, TaintStore>& memo)
{
	for (auto const& record: sinkPoints)
	{
		errs() << *record.context << ", " << *record.inst << ", " << record.callee->getName() << "\n";
		ImmutableCallSite cs(record.inst);
		assert(cs);

		auto summary = ssManager.getSummary(record.callee->getName());
		if (summary == nullptr)
			continue;

		auto itr = memo.find(ProgramLocation(record.context, record.inst));
		auto const& store = (itr == memo.end()) ? TaintStore() : itr->second;
		if (!checkValue(*summary, record.context, cs, env, store))
			return false;
	}

	return true;
}

}
}
