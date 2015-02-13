#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/CallSite.h>

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

void evalExternalCall(const PointerCFGNode* node, const Function* f, ReachingDefStore& store, const ExternalPointerEffectTable& extTable, const PointerAnalysis& ptrAnalysis)
{
	ImmutableCallSite cs(node->getInstruction());
	assert(cs);

	auto extType = extTable.lookup(f->getName());
	switch (extType)
	{
		case PointerEffect::StoreArg0ToArg1:
		{
			auto dstVal = cs.getArgument(1);

			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
			{
				for (auto loc: *pSet)
					store.insertBinding(loc, node);
			}

			break;
		}
		case PointerEffect::MemcpyArg0ToArg1:
		{
			auto dstVal = cs.getArgument(1);

			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
			{
				for (auto loc: *pSet)
				{
					for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
						store.insertBinding(oLoc, node);
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
					for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
						store.insertBinding(oLoc, node);
				}
			}


			break;
		}
		case PointerEffect::UnknownEffect:
			llvm_unreachable("Unknown external call");
		default:
			break;
	}
}

}

ReachingDefStore::NodeSet& ReachingDefStore::getReachingDefs(const MemoryLocation* loc)
{
	return store[loc];
}

const ReachingDefStore::NodeSet* ReachingDefStore::getReachingDefs(const MemoryLocation* loc) const
{
	auto itr = store.find(loc);
	if (itr == store.end())
		return nullptr;
	else
		return &itr->second;
}

bool ReachingDefStore::insertBinding(const MemoryLocation* loc, const PointerCFGNode* node)
{
	auto itr = store.find(loc);
	if (itr == store.end())
		itr = store.insert(itr, std::make_pair(loc, NodeSet()));

	return itr->second.insert(node);
}

bool ReachingDefStore::mergeWith(const ReachingDefStore& rhs)
{
	bool changed = false;
	for (auto const& mapping: rhs)
	{
		auto loc = mapping.first;
		auto itr = store.find(loc);
		if (itr == store.end())
		{
			changed = true;
			store.insert(itr, mapping);
		}
		else
		{
			changed |= itr->second.mergeWith(mapping.second);
		}
	}
	return changed;
}

bool ReachingDefMap::update(const PointerCFGNode* node, const ReachingDefStore& storeUpdate)
{
	auto itr = rdStore.find(node);
	if (itr == rdStore.end())
	{
		rdStore.insert(std::make_pair(node, storeUpdate));
		return true;
	}
	else
		return itr->second.mergeWith(storeUpdate);
}

ReachingDefStore& ReachingDefMap::getReachingDefStore(const PointerCFGNode* node)
{
	return rdStore[node];
}

const ReachingDefStore& ReachingDefMap::getReachingDefStore(const PointerCFGNode* node) const
{
	auto itr = rdStore.find(node);
	assert(itr != rdStore.end());
	return itr->second;
}

void ReachingDefAnalysis::evalNode(const PointerCFGNode* node, ReachingDefStore& retStore)
{
	switch (node->getType())
	{
		case PointerCFGNodeType::Entry:
		{
			auto entryNode = cast<EntryNode>(node);
			auto& summary = summaryMap.getSummary(entryNode->getFunction());

			ReachingDefStore initStore;
			for (auto loc: summary.mem_reads())
				retStore.insertBinding(loc, node);

			break;
		}
		case PointerCFGNodeType::Alloc:
		{
			auto pSet = ptrAnalysis.getPtsSet(node->getInstruction());
			assert(pSet != nullptr && pSet->getSize() == 1);

			retStore.insertBinding(*pSet->begin(), node);

			break;
		}
		case PointerCFGNodeType::Store:
		{
			auto storeNode = cast<StoreNode>(node);

			auto dstVal = storeNode->getDest();
			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
			{
				for (auto loc: *pSet)
					retStore.insertBinding(loc, node);
			}

			break;
		}
		case PointerCFGNodeType::Call:
		{
			auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), node->getInstruction());
			for (auto callee: callTgts)
			{
				if (callee->isDeclaration())
				{
					evalExternalCall(node, callee, retStore, extTable, ptrAnalysis);
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

ReachingDefMap ReachingDefAnalysis::runOnPointerCFG(const PointerCFG& cfg)
{
	ReachingDefMap rdMap;

	PrioWorkList<const PointerCFGNode*, PrioCompare> workList;

	workList.enqueue(cfg.getEntryNode());

	while (!workList.isEmpty())
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
