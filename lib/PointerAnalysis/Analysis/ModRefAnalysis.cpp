#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

using RevCallMapType = std::unordered_map<const Function*, util::VectorSet<const Instruction*>>;

void updateRevCallGraph(RevCallMapType& revCallGraph, const PointerCFG& cfg, const PointerAnalysis& ptrAnalysis)
{
	for (auto node: cfg)
	{
		if (auto callNode = dyn_cast<CallNode>(node))
		{
			auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), callNode->getInstruction());
			for (auto f: callTgts)
				revCallGraph[f].insert(callNode->getInstruction());
		}
	}
}

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

bool addExternalMemoryWrite(const Value* v, ModRefSummary& summary, const Function* caller, const PointerAnalysis& ptrAnalysis, bool isReachableMemory)
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

}

void ModRefAnalysis::collectProcedureSummary(const PointerCFG& cfg, ModRefSummary& summary)
{
	auto f = cfg.getFunction();

	// Search for all non-local values and memory objects and add them to the summary
	for (auto node: cfg)
	{
		switch (node->getType())
		{
			case PointerCFGNodeType::Copy:
			{
				auto copyNode = cast<CopyNode>(node);

				// The destination value must be local, but the sources may contain globals
				for (auto src: *copyNode)
					if (isa<GlobalValue>(src))
						summary.addValueRead(src);

				break;
			}
			case PointerCFGNodeType::Load:
			{
				auto loadNode = cast<LoadNode>(node);
				auto loadSrc = loadNode->getSrc();
				if (isa<GlobalValue>(loadSrc))
					summary.addValueRead(loadSrc);
				auto pSet = ptrAnalysis.getPtsSet(loadSrc);
				for (auto loc: pSet)
				{
					if (!isLocalStackLocation(loc, f))
						summary.addMemoryRead(loc);
				}

				break;
			}
			case PointerCFGNodeType::Store:
			{
				auto storeNode = cast<StoreNode>(node);
				auto storeSrc = storeNode->getSrc();
				auto storeDst = storeNode->getDest();

				if (isa<GlobalValue>(storeSrc))
					summary.addValueRead(storeSrc);
				if (isa<GlobalValue>(storeDst))
					summary.addValueRead(storeDst);

				auto pSet = ptrAnalysis.getPtsSet(storeDst);
				for (auto loc: pSet)
				{
					if (!isLocalStackLocation(loc, f))
						summary.addMemoryWrite(loc);
				}

				break;
			}
			case PointerCFGNodeType::Call:
			{
				auto callNode = cast<CallNode>(node);

				auto funPtr = callNode->getFunctionPointer();
				if (isa<GlobalValue>(funPtr))
					summary.addValueRead(funPtr);

				for (auto arg: *callNode)
					if (isa<GlobalValue>(arg))
						summary.addValueRead(arg);

				break;
			}
			case PointerCFGNodeType::Ret:
			{
				auto retNode = cast<ReturnNode>(node);

				if (auto retVal = retNode->getReturnValue())
					if (isa<GlobalValue>(retVal))
						summary.addValueRead(retVal);

				break;
			}
			default:
				break;
		}
	}
}

ModRefSummaryMap ModRefAnalysis::runOnProgram(const PointerProgram& prog)
{
	ModRefSummaryMap summaryMap;

	RevCallMapType revCallGraph;

	for (auto const& cfg: prog)
	{
		collectProcedureSummary(cfg, summaryMap.getSummary(cfg.getFunction()));
		updateRevCallGraph(revCallGraph, cfg, ptrAnalysis);
	}

	propagateSummary(summaryMap, revCallGraph, ptrAnalysis, modRefTable);

	return summaryMap;
}

}
