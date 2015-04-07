#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"

using namespace llvm;

namespace tpa
{

PtsSet PointerAnalysis::getPtsSet(const Value* val) const
{
	auto ptrs = ptrManager.getPointersWithValue(val->stripPointerCasts());
	auto retSet = PtsSet::getEmptySet();
	for (auto ptr: ptrs)
	{
		auto newSet = getPtsSet(ptr);
		if (!newSet.isEmpty())
			retSet = retSet.merge(newSet);
	}
	return retSet;
}

PtsSet PointerAnalysis::getPtsSet(const Context* ctx, const llvm::Value* val) const
{
	assert(val != nullptr);
	assert(val->getType()->isPointerTy());

	auto ptr = ptrManager.getPointer(ctx, val->stripPointerCasts());
	if (ptr == nullptr)
		return PtsSet::getEmptySet();

	return getPtsSet(ptr);
}

PtsSet PointerAnalysis::getPtsSet(const Pointer* ptr) const
{
	assert(ptr != nullptr);

	auto pSet = env.lookup(ptr);
	assert(!pSet.isEmpty());

	return pSet;
}

std::vector<const llvm::Function*> PointerAnalysis::getCallTargets(const Context* ctx, const llvm::Instruction* inst) const
{
	auto retVec = std::vector<const llvm::Function*>();

	ImmutableCallSite cs(inst);
	assert(cs && "getCallTargets() gets a non-call inst");
	if (auto f = cs.getCalledFunction())
		retVec.push_back(f);
	else
	{
		for (auto const& callTgt: callGraph.getCallTargets(std::make_pair(ctx, inst)))
			retVec.push_back(callTgt.second);

		std::sort(retVec.begin(), retVec.end());
		retVec.erase(std::unique(retVec.begin(), retVec.end()), retVec.end());
	}
	
	return retVec;
}

}
