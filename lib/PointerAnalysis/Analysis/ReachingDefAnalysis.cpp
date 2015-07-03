#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

struct PrioCompare
{
	bool operator()(const PointerCFGNode* n0, const PointerCFGNode* n1) const
	{
		return n0->getPriority() < n1->getPriority();
	}
};

void modValue(const Value* v, ReachingDefStore<PointerCFGNode>& store, const PointerCFGNode* node, const PointerAnalysis& ptrAnalysis, bool isReachableMemory)
{
	auto pSet = ptrAnalysis.getPtsSet(v);
	for (auto loc: pSet)
	{
		if (isReachableMemory)
		{
			for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
				store.insertBinding(oLoc, node);
		}
		else
		{
			store.insertBinding(loc, node);
		}
	}
}

void evalExternalCall(const PointerCFGNode* node, const Function* f, ReachingDefStore<PointerCFGNode>& store, const ExternalModRefTable& modRefTable, const PointerAnalysis& ptrAnalysis)
{
	ImmutableCallSite cs(node->getInstruction());
	assert(cs);

	auto summary = modRefTable.lookup(f->getName());
	if (summary == nullptr)
	{
		errs() << "Missing entry in ModRefTable: " << f->getName() << "\n";
		llvm_unreachable("Consider adding the function to modref annotations");
	}

	for (auto const& effect: *summary)
	{
		if (effect.isModEffect())
		{
			auto const& pos = effect.getPosition();
			if (pos.isReturnPosition())
			{
				modValue(node->getInstruction(), store, node, ptrAnalysis, effect.onReachableMemory());
			}
			else
			{
				auto const& argPos = pos.getAsArgPosition();
				unsigned idx = argPos.getArgIndex();

				if (!argPos.isAfterArgPosition())
					modValue(cs.getArgument(idx)->stripPointerCasts(), store, node, ptrAnalysis, effect.onReachableMemory());
				else
				{
					for (auto i = idx, e = cs.arg_size(); i < e; ++i)
						modValue(cs.getArgument(i)->stripPointerCasts(), store, node, ptrAnalysis, effect.onReachableMemory());
				}
			}
		}
	}
}

}

void ReachingDefAnalysis::evalNode(const PointerCFGNode* node, ReachingDefStore<PointerCFGNode>& retStore)
{
	switch (node->getType())
	{
		case PointerCFGNodeType::Entry:
		{
			auto entryNode = cast<EntryNode>(node);
			auto& summary = summaryMap.getSummary(entryNode->getFunction());

			ReachingDefStore<PointerCFGNode> initStore;
			for (auto loc: summary.mem_reads())
				retStore.insertBinding(loc, node);

			break;
		}
		case PointerCFGNodeType::Alloc:
		{
			auto pSet = ptrAnalysis.getPtsSet(node->getInstruction());
			assert(pSet.size() == 1);

			retStore.insertBinding(*pSet.begin(), node);

			break;
		}
		case PointerCFGNodeType::Store:
		{
			auto storeNode = cast<StoreNode>(node);

			auto dstVal = storeNode->getDest();
			auto pSet = ptrAnalysis.getPtsSet(dstVal);
			for (auto loc: pSet)
				retStore.insertBinding(loc, node);

			break;
		}
		case PointerCFGNodeType::Call:
		{
			auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), node->getInstruction());
			for (auto callee: callTgts)
			{
				if (callee->isDeclaration())
				{
					evalExternalCall(node, callee, retStore, modRefTable, ptrAnalysis);
				}
				else
				{
					auto& summary = summaryMap.getSummary(callee);
					for (auto loc: summary.mem_writes())
						retStore.insertBinding(loc, node);
				}
			}

			break;
		}
		default:
			break;
	}
}

ReachingDefMap<PointerCFGNode> ReachingDefAnalysis::runOnPointerCFG(const PointerCFG& cfg)
{
	ReachingDefMap<PointerCFGNode> rdMap;

	PrioWorkList<const PointerCFGNode*, PrioCompare> workList;

	workList.enqueue(cfg.getEntryNode());

	while (!workList.empty())
	{
		auto node = workList.dequeue();
		auto store = rdMap.getReachingDefStore(node);
		evalNode(node, store);
		for (auto succ: node->succs())
		{
			if (rdMap.update(succ, store))
				workList.enqueue(succ);
		}
		for (auto succ: node->uses())
		{
			if (rdMap.update(succ, store))
				workList.enqueue(succ);
		}
	}

	return rdMap;
}

}
