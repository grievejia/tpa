#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/ModRefModuleAnalysis.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "PointerAnalysis/External/ExternalRefTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>

using namespace llvm;

namespace tpa
{

using RevCallMapType = std::unordered_map<const Function*, VectorSet<const Instruction*>>;

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

bool updateSummaryForExternalCall(const Instruction* inst, const Function* f, ModRefSummary& summary, const PointerAnalysis& ptrAnalysis, const ExternalModTable& extModTable, const ExternalRefTable& extRefTable)
{
	ImmutableCallSite cs(inst);
	assert(cs);

	auto caller = inst->getParent()->getParent();
	auto extModType = extModTable.lookup(f->getName());
	auto addMemWrite = [&ptrAnalysis, &summary, caller] (const llvm::Value* v, bool array = false)
	{
		auto pSet = ptrAnalysis.getPtsSet(v);
		for (auto loc: pSet)
		{
			if (isLocalStackLocation(loc, caller))
				continue;
			if (array)
			{
				for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
					summary.addMemoryWrite(oLoc);		
			}
			else
			{
				summary.addMemoryWrite(loc);
			}
		}
	};

	switch (extModType)
	{
		case ModEffect::ModArg0:
		{
			addMemWrite(cs.getArgument(0));
			break;
		}
		case ModEffect::ModArg1:
		{
			addMemWrite(cs.getArgument(1));
			break;
		}
		case ModEffect::ModAfterArg0:
		{
			for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
				addMemWrite(cs.getArgument(i));
			break;
		}
		case ModEffect::ModAfterArg1:
		{
			for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
				addMemWrite(cs.getArgument(i));
			break;
		}
		case ModEffect::ModArg0Array:
		{
			addMemWrite(cs.getArgument(0), true);
			break;
		}
		case ModEffect::UnknownEffect:
			llvm_unreachable("Unknown mod effect");
		default:
			break;
	}

	auto extRefType = extRefTable.lookup(f->getName());
	auto addMemRead = [&ptrAnalysis, &summary, caller] (const llvm::Value* v, bool array = false)
	{
		auto pSet = ptrAnalysis.getPtsSet(v);
		for (auto loc: pSet)
		{
			if (isLocalStackLocation(loc, caller))
				continue;
			if (array)
			{
				for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
					summary.addMemoryRead(oLoc);		
			}
			else
			{
				summary.addMemoryRead(loc);
			}
		}
	};

	switch (extRefType)
	{
		case RefEffect::RefArg0:
		{
			addMemRead(cs.getArgument(0));
			break;
		}
		case RefEffect::RefArg1:
		{
			addMemRead(cs.getArgument(1));
			break;
		}
		case RefEffect::RefArg0Arg1:
		{
			addMemRead(cs.getArgument(0));
			addMemRead(cs.getArgument(1));
			break;
		}
		case RefEffect::RefAfterArg0:
		{
			for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
				addMemRead(cs.getArgument(i));
			break;
		}
		case RefEffect::RefAfterArg1:
		{
			for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
				addMemRead(cs.getArgument(i));
			break;
		}
		case RefEffect::RefAllArgs:
		{
			for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
				addMemRead(cs.getArgument(i));
			break;
		}
		case RefEffect::RefArg1Array:
		{
			addMemRead(cs.getArgument(1), true);
			break;
		}
		case RefEffect::UnknownEffect:
			llvm_unreachable("Unknown ref effect");
		default:
			break;
	}

	return false;
}

void propagateSummary(ModRefSummaryMap& summaryMap, const RevCallMapType& revCallGraph, const PointerAnalysis& ptrAnalysis, const ExternalModTable& extModTable, const ExternalRefTable& extRefTable)
{
	auto workList = WorkList<const Function*>();

	// Enqueue all callees at the beginning
	for (auto const& mapping: revCallGraph)
		workList.enqueue(mapping.first);

	// Each function "push"es its info back into its callers
	while (!workList.isEmpty())
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
				if (updateSummaryForExternalCall(callSite, f, callerSummary, ptrAnalysis, extModTable, extRefTable))
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

	propagateSummary(summaryMap, revCallGraph, ptrAnalysis, extModTable, extRefTable);

	return summaryMap;
}

}