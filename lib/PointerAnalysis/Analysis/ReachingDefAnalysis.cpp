#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ExternalModTable.h"
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

void evalExternalCall(const PointerCFGNode* node, const Function* f, ReachingDefStore<PointerCFGNode>& store, const ExternalModTable& extTable, const PointerAnalysis& ptrAnalysis)
{
	ImmutableCallSite cs(node->getInstruction());
	assert(cs);

	auto extType = extTable.lookup(f->getName());
	auto modArg = [&ptrAnalysis, &store, node] (const llvm::Value* v, bool array = false)
	{
		auto pSet = ptrAnalysis.getPtsSet(v);
		for (auto loc: pSet)
		{
			if (array)
			{
				for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
					store.insertBinding(oLoc, node);
			}
			else
			{
				store.insertBinding(loc, node);
			}
		}
	};

	switch (extType)
	{
		case ModEffect::ModArg0:
		{
			modArg(cs.getArgument(0));
			break;
		}
		case ModEffect::ModArg1:
		{
			modArg(cs.getArgument(1));
			break;
		}
		case ModEffect::ModAfterArg0:
		{
			for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
				modArg(cs.getArgument(i));
			break;
		}
		case ModEffect::ModAfterArg1:
		{
			for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
				modArg(cs.getArgument(i));
			break;
		}
		case ModEffect::ModArg0Array:
		{
			modArg(cs.getArgument(0), true);

			break;
		}
		case ModEffect::UnknownEffect:
			llvm_unreachable("Unknown mod effect");
		default:
			break;
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
			assert(pSet.getSize() == 1);

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
					evalExternalCall(node, callee, retStore, extModTable, ptrAnalysis);
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
