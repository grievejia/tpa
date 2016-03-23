#include "Annotation/ModRef/ExternalModRefTable.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/MemoryModel/MemoryObject.h"
#include "TaintAnalysis/FrontEnd/ModRefModuleAnalysis.h"
#include "Util/DataStructure/FIFOWorkList.h"
#include "Util/DataStructure/VectorSet.h"

#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;
using namespace tpa;

namespace taint
{

using RevCallMapType = std::unordered_map<const Function*, util::VectorSet<const Instruction*>>;

namespace
{

inline bool isLocalStackLocation(const MemoryObject* loc, const Function* f)
{
	auto const& allocSite = loc->getAllocSite();

	switch (allocSite.getAllocType())
	{
		// NullObj and UniversalObj are counted as local stack location because we don't want them to appear in the mod-ref summaries
		case AllocSiteTag::Null:
		case AllocSiteTag::Universal:
			return true;
		case AllocSiteTag::Global:
		case AllocSiteTag::Function:
		case AllocSiteTag::Heap:
			return false;
		case AllocSiteTag::Stack:
		{
			if (auto allocInst = dyn_cast<AllocaInst>(allocSite.getLocalValue()))
				return allocInst->getParent()->getParent() == f;
			else
				return false;
		}
	}
}

bool updateSummary(ModRefFunctionSummary& callerSummary, const ModRefFunctionSummary& calleeSummary, const Function* caller)
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

void updateRevCallGraph(RevCallMapType& revCallGraph, const Function& f, const SemiSparsePointerAnalysis& ptrAnalysis)
{
	for (auto const& bb: f)
	{
		for (auto const& inst: bb)
		{
			ImmutableCallSite cs(&inst);
			if (!cs)
				continue;

			auto callTgts = ptrAnalysis.getCallees(cs);
			for (auto callee: callTgts)
				revCallGraph[callee].insert(cs.getInstruction());
		}
	}
}

class SummaryInstVisitor: public InstVisitor<SummaryInstVisitor>
{
private:
	const SemiSparsePointerAnalysis& ptrAnalysis;

	ModRefFunctionSummary& summary;
public:
	SummaryInstVisitor(const SemiSparsePointerAnalysis& p, ModRefFunctionSummary& s): ptrAnalysis(p), summary(s) {}

	void visitLoadInst(LoadInst& loadInst)
	{
		auto ptrOp = loadInst.getPointerOperand();
		if (isa<GlobalValue>(ptrOp))
			summary.addValueRead(ptrOp->stripPointerCasts());
		auto pSet = ptrAnalysis.getPtsSet(ptrOp);
		for (auto obj: pSet)
		{
			if (!isLocalStackLocation(obj, loadInst.getParent()->getParent()))
				summary.addMemoryRead(obj);
		}
	}

	void visitStoreInst(StoreInst& storeInst)
	{
		auto storeSrc = storeInst.getValueOperand();
		auto storeDst = storeInst.getPointerOperand();

		if (isa<GlobalValue>(storeSrc))
			summary.addValueRead(storeSrc->stripPointerCasts());
		if (isa<GlobalValue>(storeDst))
			summary.addValueRead(storeDst->stripPointerCasts());

		auto pSet = ptrAnalysis.getPtsSet(storeDst);
		for (auto obj: pSet)
		{
			if (!isLocalStackLocation(obj, storeInst.getParent()->getParent()))
				summary.addMemoryWrite(obj);
		}
	}

	void visitInstruction(Instruction& inst)
	{
		for (auto& use: inst.operands())
		{
			auto value = use.get();
			if (isa<GlobalValue>(value))
				summary.addValueRead(value->stripPointerCasts());
		}
	}
};

bool addExternalMemoryWrite(const Value* v, ModRefFunctionSummary& summary, const Function* caller, const SemiSparsePointerAnalysis& ptrAnalysis, bool isReachableMemory)
{
	auto changed = false;
	auto pSet = ptrAnalysis.getPtsSet(v);
	for (auto loc: pSet)
	{
		if (isLocalStackLocation(loc, caller))
			continue;
		if (isReachableMemory)
		{
			for (auto oLoc: ptrAnalysis.getMemoryManager().getReachableMemoryObjects(loc))
				changed |= summary.addMemoryWrite(oLoc);		
		}
		else
		{
			changed |= summary.addMemoryWrite(loc);
		}
	}
	return changed;
}

bool updateSummaryForModEffect(const Instruction* inst, ModRefFunctionSummary& summary, const SemiSparsePointerAnalysis& ptrAnalysis, const ModRefEffect& modEffect)
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

bool addExternalMemoryRead(const Value* v, ModRefFunctionSummary& summary, const Function* caller, const SemiSparsePointerAnalysis& ptrAnalysis, bool isReachableMemory)
{
	bool changed = false;
	auto pSet = ptrAnalysis.getPtsSet(v);
	for (auto loc: pSet)
	{
		if (isLocalStackLocation(loc, caller))
			continue;
		if (isReachableMemory)
		{
			for (auto oLoc: ptrAnalysis.getMemoryManager().getReachableMemoryObjects(loc))
				changed |= summary.addMemoryRead(oLoc);		
		}
		else
		{
			changed |= summary.addMemoryRead(loc);
		}
	}
	return changed;
}

bool updateSummaryForRefEffect(const Instruction* inst, ModRefFunctionSummary& summary, const SemiSparsePointerAnalysis& ptrAnalysis, const ModRefEffect& refEffect)
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
		{
			changed |= addExternalMemoryRead(cs.getArgument(i)->stripPointerCasts(), summary, caller, ptrAnalysis, refEffect.onReachableMemory());
		}
		return changed;
	}
}

bool updateSummaryForExternalCall(const Instruction* inst, const Function* f, ModRefFunctionSummary& summary, const SemiSparsePointerAnalysis& ptrAnalysis, const ExternalModRefTable& modRefTable)
{
	auto modRefSummary = modRefTable.lookup(f->getName());
	if (modRefSummary == nullptr)
	{
		errs() << modRefTable.size() << "\n";
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

void propagateSummary(ModRefModuleSummary& moduleSummary, const RevCallMapType& revCallGraph, const SemiSparsePointerAnalysis& ptrAnalysis, const ExternalModRefTable& modRefTable)
{
	auto workList = util::FIFOWorkList<const Function*>();

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
				auto& callerSummary = moduleSummary.getSummary(caller);
				if (updateSummaryForExternalCall(callSite, f, callerSummary, ptrAnalysis, modRefTable))
					workList.enqueue(caller);
			}
		}
		else
		{
			auto& calleeSummary = moduleSummary.getSummary(f);
			for (auto callSite: callerSet)
			{
				auto caller = callSite->getParent()->getParent();
				auto& callerSummary = moduleSummary.getSummary(caller);
				if (updateSummary(callerSummary, calleeSummary, caller))
					workList.enqueue(caller);
			}

		}
	}
}

}	// end of anonymous namespace

void ModRefModuleAnalysis::collectProcedureSummary(const Function& f, ModRefFunctionSummary& summary)
{
	SummaryInstVisitor summaryVisitor(ptrAnalysis, summary);
	summaryVisitor.visit(const_cast<Function&>(f));
}

ModRefModuleSummary ModRefModuleAnalysis::runOnModule(const Module& module)
{
	ModRefModuleSummary moduleSummary;
	RevCallMapType revCallGraph;

	for (auto const& f: module)
	{
		if (f.isDeclaration())
			continue;
		collectProcedureSummary(f, moduleSummary.getSummary(&f));
		updateRevCallGraph(revCallGraph, f, ptrAnalysis);
	}

	propagateSummary(moduleSummary, revCallGraph, ptrAnalysis, modRefTable);

	return moduleSummary;
}

}