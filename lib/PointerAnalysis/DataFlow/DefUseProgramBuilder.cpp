#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

using NodeMap = DenseMap<const PointerCFGNode*, DefUseGraphNode*>;

inline DefUseGraphNode* getDefUseNode(const NodeMap& nodeMap, const PointerCFGNode* node)
{
	auto itr = nodeMap.find(node);
	assert(itr != nodeMap.end());
	return itr->second;
}

DefUseGraphNode* createDefUseNodeFrom(DefUseGraph& dug, const PointerCFGNode* node)
{
	switch (node->getType())
	{
		case PointerCFGNodeType::Entry:
			return dug.create<EntryDefUseNode>(dug.getFunction());
		case PointerCFGNodeType::Alloc:
			return dug.create<AllocDefUseNode>(node->getInstruction());
		case PointerCFGNodeType::Copy:
		{
			auto copyNode = cast<CopyNode>(node);
			if (copyNode->getOffset() == 0)
			{
				auto retNode = dug.create<CopyDefUseNode>(copyNode->getInstruction());
				for (auto src: *copyNode)
					retNode->addSrc(src);
				return retNode;
			}
			else
				return dug.create<CopyDefUseNode>(copyNode->getInstruction(), copyNode->getFirstSrc(), copyNode->getOffset(), copyNode->isArrayRef());
		}
		case PointerCFGNodeType::Load:
		{
			auto loadNode = cast<LoadNode>(node);
			return dug.create<LoadDefUseNode>(loadNode->getInstruction(), loadNode->getSrc());
		}
		case PointerCFGNodeType::Store:
		{
			auto storeNode = cast<StoreNode>(node);
			return dug.create<StoreDefUseNode>(storeNode->getInstruction(), storeNode->getDest(), storeNode->getSrc());
		}
		case PointerCFGNodeType::Call:
		{
			auto callNode = cast<CallNode>(node);
			auto retNode = dug.create<CallDefUseNode>(callNode->getInstruction(), callNode->getFunctionPointer(), callNode->getDest());
			for (auto arg: *callNode)
				retNode->addArgument(arg);
			return retNode;
		}
		case PointerCFGNodeType::Ret:
		{
			auto retNode = cast<ReturnNode>(node);
			return dug.create<ReturnDefUseNode>(retNode->getInstruction(), retNode->getReturnValue());
		}
	}
}

void buildTopLevelEdges(const PointerCFG& cfg, const NodeMap& nodeMap)
{
	for (auto srcNode: cfg)
	{
		for (auto dstNode: srcNode->uses())
		{
			auto srcDefUseNode = getDefUseNode(nodeMap, srcNode);
			auto dstDefUseNode = getDefUseNode(nodeMap, dstNode);
			srcDefUseNode->insertTopLevelEdge(dstDefUseNode);
		}
	}
}

void processValueRef(const Value* v, const NodeMap& nodeMap, const ReachingDefStore<PointerCFGNode>& rdStore, DefUseGraphNode* dstDefUseNode, const PointerAnalysis& ptrAnalysis, bool isReachableMemory)
{
	auto refLoc = [&rdStore, &nodeMap, dstDefUseNode, &ptrAnalysis] (const MemoryLocation* loc)
	{
		auto nodeSet = rdStore.getReachingDefs(loc);
		if (nodeSet == nullptr)
			return;
		for (auto srcNode: *nodeSet)
		{
			auto srcDefUseNode = getDefUseNode(nodeMap, srcNode);
			srcDefUseNode->insertMemLevelEdge(loc, dstDefUseNode);
		}
	};

	auto pSet = ptrAnalysis.getPtsSet(v);
	for (auto loc: pSet)
	{
		if (isReachableMemory)
		{
			for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
				refLoc(oLoc);
		}
		else
		{
			refLoc(loc);
		}
	}
}

void processMemReadForExternalCall(const PointerCFGNode* node, const Function* callee, DefUseGraphNode* dstDefUseNode, const ExternalModRefTable& modRefTable, const NodeMap& nodeMap, const ReachingDefStore<PointerCFGNode>& rdStore, const PointerAnalysis& ptrAnalysis)
{
	ImmutableCallSite cs(node->getInstruction());
	assert(cs);

	auto summary = modRefTable.lookup(callee->getName());
	if (summary == nullptr)
	{
		errs() << "Missing entry in ModRefTable: " << callee->getName() << "\n";
		llvm_unreachable("Consider adding the function to modref annotations");
	}

	for (auto const& effect: *summary)
	{
		if (effect.isRefEffect())
		{
			auto const& pos = effect.getPosition();
			assert(!pos.isReturnPosition() && "It doesn't make any sense to ref a return position!");

			auto const& argPos = pos.getAsArgPosition();
			unsigned idx = argPos.getArgIndex();

			if (!argPos.isAfterArgPosition())
				processValueRef(cs.getArgument(idx)->stripPointerCasts(),nodeMap, rdStore, dstDefUseNode, ptrAnalysis, effect.onReachableMemory());
			else
			{
				for (auto i = idx, e = cs.arg_size(); i < e; ++i)
					processValueRef(cs.getArgument(i)->stripPointerCasts(),nodeMap, rdStore, dstDefUseNode, ptrAnalysis, effect.onReachableMemory());
			}
		}
	}
}

void buildMemLevelEdges(const PointerCFG& cfg, const NodeMap& nodeMap, const ModRefSummaryMap& summaryMap, const ReachingDefMap<PointerCFGNode>& rdMap, const PointerAnalysis& ptrAnalysis, const ExternalModRefTable& modRefTable)
{
	auto processMemRead = [&nodeMap, &rdMap, &ptrAnalysis] (const PointerCFGNode* node, const llvm::Value* val)
	{
		auto pSet = ptrAnalysis.getPtsSet(val);
		if (!pSet.isEmpty())
		{
			auto& rdStore = rdMap.getReachingDefStore(node);
			auto dstDefUseNode = getDefUseNode(nodeMap, node);
			for (auto loc: pSet)
			{
				if (ptrAnalysis.getMemoryManager().isSpecialMemoryLocation(loc))
					continue;
				auto nodeSet = rdStore.getReachingDefs(loc);
				assert(nodeSet != nullptr);
				for (auto srcNode: *nodeSet)
				{
					auto srcDefUseNode = getDefUseNode(nodeMap, srcNode);
					srcDefUseNode->insertMemLevelEdge(loc, dstDefUseNode);
				}
			}
		}
	};

	for (auto node: cfg)
	{
		switch (node->getType())
		{
			case PointerCFGNodeType::Load:
			{
				auto loadNode = cast<LoadNode>(node);
				auto loadSrc = loadNode->getSrc();

				processMemRead(loadNode, loadSrc);

				break;
			}
			case PointerCFGNodeType::Call:
			{
				auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), node->getInstruction());
				auto& rdStore = rdMap.getReachingDefStore(node);
				auto dstDefUseNode = getDefUseNode(nodeMap, node);
				for (auto callee: callTgts)
				{
					if (callee->isDeclaration())
					{
						processMemReadForExternalCall(node, callee, dstDefUseNode, modRefTable, nodeMap, rdStore, ptrAnalysis);
					}
					else
					{
						auto& summary = summaryMap.getSummary(callee);
						for (auto loc: summary.mem_reads())
						{
							auto nodeSet = rdStore.getReachingDefs(loc);
							if (nodeSet == nullptr)
								continue;
							for (auto srcNode: *nodeSet)
							{
								auto srcDefUseNode = getDefUseNode(nodeMap, srcNode);
								srcDefUseNode->insertMemLevelEdge(loc, dstDefUseNode);
							}
						}
					}
				}

				break;
			}
			default:
				break;
		}
	}
}

}	// end of anonymous namespace

void DefUseProgramBuilder::buildDefUseGraph(DefUseGraph& dug, const PointerCFG& cfg)
{
	auto nodeMap = NodeMap();

	// Create the nodes first. We will worry about how to connect them later
	for (auto node: cfg)
	{
		auto duNode = createDefUseNodeFrom(dug, node);
		duNode->setPriority(node->getPriority());
		nodeMap[node] = duNode;
	}

	ReachingDefAnalysis rdAnalysis(ptrAnalysis, summaryMap, modRefTable);
	auto rdMap = rdAnalysis.runOnPointerCFG(cfg);

	// Set the entry and the exit
	auto entryDefUseNode = getDefUseNode(nodeMap, cfg.getEntryNode());
	auto exitDefUseNode = getDefUseNode(nodeMap, cfg.getExitNode());
	dug.setEntryNode(cast<EntryDefUseNode>(entryDefUseNode));
	dug.setExitNode(cast<ReturnDefUseNode>(exitDefUseNode));
	// This edge exists because we want the analysis to always reach the exit node
	if (!cfg.doesNotReturn())
		entryDefUseNode->insertTopLevelEdge(exitDefUseNode);

	buildTopLevelEdges(cfg, nodeMap);
	buildMemLevelEdges(cfg, nodeMap, summaryMap, rdMap, ptrAnalysis, modRefTable);
}

DefUseProgram DefUseProgramBuilder::buildDefUseProgram(const PointerProgram& prog)
{
	DefUseProgram duProg;

	for (auto const& cfg: prog)
	{
		// Create the def-use graph from PointerCFG
		auto dug = duProg.createDefUseGraphForFunction(cfg.getFunction());
		buildDefUseGraph(*dug, cfg);
		if (&cfg == prog.getEntryCFG())
			duProg.setEntryGraph(dug);
	}
	duProg.addAddrTakenFunctions(prog.at_funs());

	return duProg;
}

}
