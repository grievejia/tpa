#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

using RevCallMapType = std::unordered_map<const Function*, util::VectorSet<const Instruction*>>;

namespace
{

inline bool isLocalStackLocation(const MemoryLocation* loc, const Function* f)
{
	auto allocSite = loc->getMemoryObject()->getAllocationSite().getInstruction();
	// FIXME: NullObj and UniversalObj are counted as local stack location because we don't want them to appear in the mod-ref summaries
	if (allocSite == nullptr)
		return true;
	else if (auto allocInst = dyn_cast<AllocaInst>(allocSite))
		return allocInst->getParent()->getParent() == f;
	else
		return false;
}

bool updateSummary(ModRefSummary& callerSummary, const ModRefSummary& calleeSummary, const Function* caller)
{
	bool changed = false;

	for (auto value: calleeSummary.value_reads())
		changed |= callerSummary.addValueRead(value);
	for (auto loc: calleeSummary.mem_reads())
	{
		if (!isLocalStackLocation(loc, caller))
			changed |= callerSummary.addMemoryRead(loc);
	}
	for (auto loc: calleeSummary.mem_writes())
	{
		if (!isLocalStackLocation(loc, caller))
			changed |= callerSummary.addMemoryWrite(loc);
	}

	return changed;
}

void updateRevCallGraph(RevCallMapType& revCallGraph, const Function& f, const PointerAnalysis& ptrAnalysis)
{
	for (auto const& bb: f)
	{
		for (auto const& inst: bb)
		{
			ImmutableCallSite cs(&inst);
			if (!cs)
				continue;

			auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), cs.getInstruction());
			for (auto callee: callTgts)
				revCallGraph[callee].insert(cs.getInstruction());
		}
	}
}

class SummaryInstVisitor: public InstVisitor<SummaryInstVisitor>
{
private:
	const PointerAnalysis& ptrAnalysis;

	ModRefSummary& summary;
public:
	SummaryInstVisitor(const PointerAnalysis& p, ModRefSummary& s): ptrAnalysis(p), summary(s) {}

	void visitLoadInst(LoadInst& loadInst)
	{
		auto ptrOp = loadInst.getPointerOperand();
		if (isa<GlobalValue>(ptrOp))
			summary.addValueRead(ptrOp);
		auto pSet = ptrAnalysis.getPtsSet(ptrOp);
		for (auto loc: pSet)
		{
			if (!isLocalStackLocation(loc, loadInst.getParent()->getParent()))
				summary.addMemoryRead(loc);
		}
	}

	void visitStoreInst(StoreInst& storeInst)
	{
		auto storeSrc = storeInst.getValueOperand();
		auto storeDst = storeInst.getPointerOperand();

		if (isa<GlobalValue>(storeSrc))
			summary.addValueRead(storeSrc);
		if (isa<GlobalValue>(storeDst))
			summary.addValueRead(storeDst);

		auto pSet = ptrAnalysis.getPtsSet(storeDst);
		for (auto loc: pSet)
		{
			if (!isLocalStackLocation(loc, storeInst.getParent()->getParent()))
				summary.addMemoryWrite(loc);
		}
	}

	void visitInstruction(Instruction& inst)
	{
		for (auto& use: inst.operands())
		{
			auto value = use.get();
			if (isa<GlobalValue>(value))
				summary.addValueRead(value);
		}
	}
};

bool addExternalMemoryWrite(const Value* v, ModRefSummary& summary, const Function* caller, const PointerAnalysis& ptrAnalysis, bool isReachableMemory)
{
	auto changed = false;
	auto pSet = ptrAnalysis.getPtsSet(v);
	for (auto loc: pSet)
	{
		if (isLocalStackLocation(loc, caller))
			continue;
		if (isReachableMemory)
		{
			for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
				changed |= summary.addMemoryWrite(oLoc);		
		}
		else
		{
			changed |= summary.addMemoryWrite(loc);
		}
	}
	return changed;
}

bool updateSummaryForModEffect(const Instruction* inst, ModRefSummary& summary, const PointerAnalysis& ptrAnalysis, const ModRefEffect& modEffect)
{
	assert(modEffect.isModEffect());

	ImmutableCallSite cs(inst);
	assert(cs);
	auto caller = inst->getParent()->getParent();

	auto const& pos = modEffect.getPosition();
	if (pos.isReturnPosition())
		return addExternalMemoryWrite(inst, summary, caller, ptrAnalysis, modEffect.onReachableMemory());
	else
	{
		auto const& argPos = pos.getAsArgPosition();
		unsigned idx = argPos.getArgIndex();

		if (!argPos.isAfterArgPosition())
			return addExternalMemoryWrite(cs.getArgument(idx)->stripPointerCasts(), summary, caller, ptrAnalysis, modEffect.onReachableMemory());
		else
		{
			bool changed = false;
			for (auto i = idx, e = cs.arg_size(); i < e; ++i)
				changed |= addExternalMemoryWrite(cs.getArgument(i)->stripPointerCasts(), summary, caller, ptrAnalysis, modEffect.onReachableMemory());
			return changed;
		}
	}
}

bool addExternalMemoryRead(const Value* v, ModRefSummary& summary, const Function* caller, const PointerAnalysis& ptrAnalysis, bool isReachableMemory)
{
	bool changed = false;
	auto pSet = ptrAnalysis.getPtsSet(v);
	for (auto loc: pSet)
	{
		if (isLocalStackLocation(loc, caller))
			continue;
		if (isReachableMemory)
		{
			for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
				changed |= summary.addMemoryRead(oLoc);		
		}
		else
		{
			changed |= summary.addMemoryRead(loc);
		}
	}
	return changed;
}

bool updateSummaryForRefEffect(const Instruction* inst, ModRefSummary& summary, const PointerAnalysis& ptrAnalysis, const ModRefEffect& refEffect)
{
	assert(refEffect.isRefEffect());

	ImmutableCallSite cs(inst);
	assert(cs);
	auto caller = inst->getParent()->getParent();

	auto const& pos = refEffect.getPosition();
	assert(!pos.isReturnPosition() && "It doesn't make any sense to ref a return position!");
	
	auto const& argPos = pos.getAsArgPosition();
	unsigned idx = argPos.getArgIndex();

	if (!argPos.isAfterArgPosition())
		return addExternalMemoryRead(cs.getArgument(idx)->stripPointerCasts(), summary, caller, ptrAnalysis, refEffect.onReachableMemory());
	else
	{
		bool changed = false;
		for (auto i = idx, e = cs.arg_size(); i < e; ++i)
			changed |= addExternalMemoryRead(cs.getArgument(i)->stripPointerCasts(), summary, caller, ptrAnalysis, refEffect.onReachableMemory());
		return changed;
	}
}

bool updateSummaryForExternalCall(const Instruction* inst, const Function* f, ModRefSummary& summary, const PointerAnalysis& ptrAnalysis, const ExternalModRefTable& modRefTable)
{
	auto modRefSummary = modRefTable.lookup(f->getName());
	if (modRefSummary == nullptr)
	{
		errs() << "Missing entry in ModRefTable: " << f->getName() << "\n";
		llvm_unreachable("Consider adding the function to modref annotations");
	}

	bool changed = false;
	for (auto const& effect: *modRefSummary)
	{
		if (effect.isModEffect())
			changed |= updateSummaryForModEffect(inst, summary, ptrAnalysis, effect);
		else
			changed |= updateSummaryForRefEffect(inst, summary, ptrAnalysis, effect);
	}
	return changed;
}

void propagateSummary(ModRefSummaryMap& summaryMap, const RevCallMapType& revCallGraph, const PointerAnalysis& ptrAnalysis, const ExternalModRefTable& modRefTable)
{
	auto workList = WorkList<const Function*>();

	// Enqueue all callees at the beginning
	for (auto const& mapping: revCallGraph)
		workList.enqueue(mapping.first);

	// Each function "push"es its info back into its callers
	while (!workList.empty())
	{
		auto f = workList.dequeue();

		auto itr = revCallGraph.find(f);
		if (itr == revCallGraph.end())
			continue;
		auto& callerSet = itr->second;

		if (f->isDeclaration())
		{
			for (auto callSite: callerSet)
			{
				auto caller = callSite->getParent()->getParent();
				auto& callerSummary = summaryMap.getSummary(caller);
				if (updateSummaryForExternalCall(callSite, f, callerSummary, ptrAnalysis, modRefTable))
					workList.enqueue(caller);
			}
		}
		else
		{
			auto& calleeSummary = summaryMap.getSummary(f);
			for (auto callSite: callerSet)
			{
				auto caller = callSite->getParent()->getParent();
				auto& callerSummary = summaryMap.getSummary(caller);
				if (updateSummary(callerSummary, calleeSummary, caller))
					workList.enqueue(caller);
			}

		}
	}
}

}	// end of anonymous namespace

void ModRefModuleAnalysis::collectProcedureSummary(const Function& f, ModRefSummary& summary)
{
	SummaryInstVisitor summaryVisitor(ptrAnalysis, summary);
	summaryVisitor.visit(const_cast<Function&>(f));
}

ModRefSummaryMap ModRefModuleAnalysis::runOnModule(const Module& module)
{
	ModRefSummaryMap summaryMap;

	RevCallMapType revCallGraph;

	for (auto const& f: module)
	{
		if (f.isDeclaration())
			continue;
		collectProcedureSummary(f, summaryMap.getSummary(&f));
		updateRevCallGraph(revCallGraph, f, ptrAnalysis);
	}

	propagateSummary(summaryMap, revCallGraph, ptrAnalysis, modRefTable);

	return summaryMap;
}

}