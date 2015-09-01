#pragma once

#include "Annotation/Pointer/ExternalPointerTable.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/Support/PtsSet.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Module.h>

namespace context
{
	class Context;
}

namespace tpa
{

// CRTP pointer analysis base class
template <typename SubClass>
class PointerAnalysis
{
protected:
	PointerManager ptrManager;
	MemoryManager memManager;
	annotation::ExternalPointerTable extTable;

	void getCallees(const llvm::ImmutableCallSite& cs, PtsSet pSet, std::vector<const llvm::Function*>& funcs) const
	{
		if (pSet.has(MemoryManager::getUniversalObject()))
		{
			auto module = cs.getInstruction()->getParent()->getParent()->getParent();
			for (auto const& f: *module)
				if (f.hasAddressTaken())
					funcs.push_back(&f);
		}
		else
		{
			for (auto obj: pSet)
			{
				auto& allocSite = obj->getAllocSite();
				if (allocSite.getAllocType() == AllocSiteTag::Function)
					funcs.push_back(allocSite.getFunction());
			}
		}
	}
public:
	PointerAnalysis() = default;

	PointerAnalysis(const PointerAnalysis&) = delete;
	PointerAnalysis(PointerAnalysis&&) noexcept = default;
	PointerAnalysis& operator=(const PointerAnalysis&) = delete;
	PointerAnalysis& operator=(PointerAnalysis&&) = delete;

	const PointerManager& getPointerManager() const { return ptrManager; }
	const MemoryManager& getMemoryManager() const { return memManager; }

	void loadExternalPointerTable(const char* extFileName)
	{
		extTable = annotation::ExternalPointerTable::loadFromFile(extFileName);
	}

	PtsSet getPtsSet(const Pointer* ptr) const
	{
		return static_cast<const SubClass*>(this)->getPtsSetImpl(ptr);
	}

	PtsSet getPtsSet(const context::Context* ctx, const llvm::Value* val) const
	{
		assert(ctx != nullptr && val != nullptr);

		auto ptr = ptrManager.getPointer(ctx, val->stripPointerCasts());
		if (ptr == nullptr)
			return PtsSet::getEmptySet();

		return getPtsSet(ptr);
	}

	PtsSet getPtsSet(const llvm::Value* val) const
	{
		assert(val != nullptr);

		auto ptrs = ptrManager.getPointersWithValue(val->stripPointerCasts());
		assert(!ptrs.empty());

		std::vector<PtsSet> pSets;
		pSets.reserve(ptrs.size());

		for (auto ptr: ptrs)
			pSets.emplace_back(getPtsSet(ptr));

		return PtsSet::mergeAll(pSets);
	}

	std::vector<const llvm::Function*> getCallees(const llvm::ImmutableCallSite& cs, const context::Context* ctx = nullptr) const
	{
		std::vector<const llvm::Function*> ret;

		if (auto f = cs.getCalledFunction())
		{
			ret.push_back(f);
		}
		else
		{
			auto funPtrVal = cs.getCalledValue();
			assert(funPtrVal != nullptr);
			
			auto pSet = ctx == nullptr ? getPtsSet(funPtrVal) : getPtsSet(ctx, funPtrVal);
			getCallees(cs, pSet, ret);
		}

		return ret;
	}
};

}