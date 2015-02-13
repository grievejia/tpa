#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

using RevCallMapType = std::unordered_map<const Function*, VectorSet<const Instruction*>>;

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
	if (auto allocInst = dyn_cast<AllocaInst>(allocSite))
	{
		return allocInst->getParent()->getParent() == f;
	}
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

bool updateSummaryForExternalCall(const Instruction* inst, const Function* f, ModRefSummary& summary, const PointerAnalysis& ptrAnalysis, const ExternalPointerEffectTable& extTable)
{
	ImmutableCallSite cs(inst);
	assert(cs);

	auto caller = inst->getParent()->getParent();
	auto extType = extTable.lookup(f->getName());

	switch (extType)
	{
		case PointerEffect::StoreArg0ToArg1:
		{
			auto dstVal = cs.getArgument(1);
			auto srcVal = cs.getArgument(0);

			if (auto pSet = ptrAnalysis.getPtsSet(srcVal))
			{
				for (auto loc: *pSet)
				{
					if (!isLocalStackLocation(loc, caller))
						summary.addMemoryRead(loc);
				}
			}
			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
			{
				for (auto loc: *pSet)
				{
					if (!isLocalStackLocation(loc, caller))
						summary.addMemoryWrite(loc);
				}
			}

			break;
		}
		case PointerEffect::MemcpyArg0ToArg1:
		{
			auto dstVal = cs.getArgument(0);
			auto srcVal = cs.getArgument(1);

			if (auto pSet = ptrAnalysis.getPtsSet(srcVal))
			{
				for (auto loc: *pSet)
				{
					if (isLocalStackLocation(loc, caller))
						continue;
					for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
						summary.addMemoryRead(oLoc);
				}
			}
			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
			{
				for (auto loc: *pSet)
				{
					if (isLocalStackLocation(loc, caller))
						continue;
					for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
						summary.addMemoryWrite(oLoc);
				}
			}

			break;
		}
		case PointerEffect::Memset:
		{
			auto dstVal = cs.getArgument(0);

			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
			{
				for (auto loc: *pSet)
				{
					if (isLocalStackLocation(loc, caller))
						continue;
					for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
						summary.addMemoryWrite(oLoc);
				}
			}


			break;
		}
		case PointerEffect::UnknownEffect:
			llvm_unreachable("Unknown external call");
		default:
			break;
	}

	return false;
}

void propagateSummary(ModRefSummaryMap& summaryMap, const RevCallMapType& revCallGraph, const PointerAnalysis& ptrAnalysis, const ExternalPointerEffectTable& extTable)
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
				if (updateSummaryForExternalCall(callSite, f, callerSummary, ptrAnalysis, extTable))
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

				if (auto pSet = ptrAnalysis.getPtsSet(loadSrc))
				{
					for (auto loc: *pSet)
					{
						if (!isLocalStackLocation(loc, f))
							summary.addMemoryRead(loc);
					}
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

				if (auto pSet = ptrAnalysis.getPtsSet(storeSrc))
				{
					for (auto loc: *pSet)
					{
						if (!isLocalStackLocation(loc, f))
							summary.addMemoryRead(loc);
					}
				}
				if (auto pSet = ptrAnalysis.getPtsSet(storeDst))
				{
					for (auto loc: *pSet)
					{
						if (!isLocalStackLocation(loc, f))
							summary.addMemoryWrite(loc);
					}
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

	propagateSummary(summaryMap, revCallGraph, ptrAnalysis, extTable);

	return summaryMap;
}

}
