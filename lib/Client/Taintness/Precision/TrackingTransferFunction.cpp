#include "MemoryModel/Memory/MemoryManager.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/TrackingHelper.h"
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

TrackingTransferFunction::TrackingTransferFunction(const TaintGlobalState& g, const Context* c): globalState(g), ctx(c)
{
}

TaintLattice TrackingTransferFunction::getTaintForValue(const Value* val)
{
	return globalState.getEnv().lookup(ProgramLocation(ctx, val));
}

ValueSet TrackingTransferFunction::evalAllOperands(const DefUseInstruction* duInst)
{
	ValueSet valueSet;

	// Obtain all values
	auto inst = duInst->getInstruction();
	std::vector<const Value*> values;
	values.reserve(inst->getNumOperands());
	for (auto const& use: inst->operands())
		values.push_back(use.get());

	propagateImprecision(
		values,
		[this] (const Value* val)
		{
			return getTaintForValue(val);
		},
		[&valueSet] (const Value* val)
		{
			if (!isa<GlobalValue>(val))
				valueSet.insert(val);
		}
	);

	return valueSet;
}

MemorySet TrackingTransferFunction::evalPtsSet(const DefUseInstruction* duInst, const PtsSet& ptsSet)
{
	MemorySet memSet;

	auto& memManager = globalState.getPointerAnalysis().getMemoryManager();
	auto store = globalState.getMemo().lookup(DefUseProgramLocation(ctx, duInst));
	assert(store != nullptr);

	// Remove uLoc and nLoc
	std::vector<const MemoryLocation*> validLocs;
	validLocs.reserve(ptsSet.getSize());
	for (auto loc: ptsSet)
		if (!memManager.isSpecialMemoryLocation(loc))
			validLocs.push_back(loc);

	propagateImprecision(
		validLocs,
		[store] (const MemoryLocation* loc)
		{
			return store->lookup(loc);
		},
		[&memSet] (const MemoryLocation* loc)
		{
			memSet.insert(loc);
		}
	);

	return memSet;
}

MemorySet TrackingTransferFunction::evalLoad(const DefUseInstruction* duInst)
{
	auto loadInst = cast<LoadInst>(duInst->getInstruction());

	auto ptrOp = loadInst->getPointerOperand();
	auto ptsSet = globalState.getPointerAnalysis().getPtsSet(ctx, ptrOp);
	assert(!ptsSet.isEmpty());

	return evalPtsSet(duInst, ptsSet);
}

ValueSet TrackingTransferFunction::evalStore(const DefUseInstruction* duInst)
{
	ValueSet valueSet;

	auto storeInst = cast<StoreInst>(duInst->getInstruction());
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

	return valueSet;
}

}
}