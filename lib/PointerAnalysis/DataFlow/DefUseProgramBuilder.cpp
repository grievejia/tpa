#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/CallSite.h>

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

void processMemReadForExternalCall(const PointerCFGNode* node, const Function* callee, DefUseGraphNode* dstDefUseNode, const ExternalPointerEffectTable& extTable, const NodeMap& nodeMap, const ReachingDefStore& rdStore, const PointerAnalysis& ptrAnalysis)
{
	ImmutableCallSite cs(node->getInstruction());
	assert(cs);

	auto processMemRead = [&rdStore, &nodeMap, dstDefUseNode] (const PtsSet* pSet)
	{
		for (auto loc: *pSet)
		{
			auto nodeSet = rdStore.getReachingDefs(loc);
			assert(nodeSet != nullptr);
			for (auto srcNode: *nodeSet)
			{
				auto srcDefUseNode = getDefUseNode(nodeMap, srcNode);
				srcDefUseNode->insertMemLevelEdge(loc, dstDefUseNode);
			}
		}
	};

	auto extType = extTable.lookup(callee->getName());
	switch (extType)
	{
		case PointerEffect::StoreArg0ToArg1:
		{
			auto srcVal = cs.getArgument(0);

			if (auto pSet = ptrAnalysis.getPtsSet(srcVal))
				processMemRead(pSet);

			break;
		}
		case PointerEffect::MemcpyArg0ToArg1:
		{
			auto dstVal = cs.getArgument(0);

			if (auto pSet = ptrAnalysis.getPtsSet(dstVal))
				processMemRead(pSet);

			break;
		}
		case PointerEffect::UnknownEffect:
			llvm_unreachable("Unknown external call");
		default:
			break;
	}
}

void buildMemLevelEdges(const PointerCFG& cfg, const NodeMap& nodeMap, const ModRefSummaryMap& summaryMap, const ReachingDefMap& rdMap, const PointerAnalysis& ptrAnalysis, const ExternalPointerEffectTable& extTable)
{
	auto processMemRead = [&nodeMap, &rdMap, &ptrAnalysis] (const PointerCFGNode* node, const llvm::Value* val)
	{
		if (auto pSet = ptrAnalysis.getPtsSet(val))
		{
			auto& rdStore = rdMap.getReachingDefStore(node);
			auto dstDefUseNode = getDefUseNode(nodeMap, node);
			for (auto loc: *pSet)
			{
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
			case PointerCFGNodeType::Store:
			{
				auto storeNode = cast<StoreNode>(node);
				auto storeSrc = storeNode->getSrc();

				processMemRead(storeNode, storeSrc);

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
						processMemReadForExternalCall(node, callee, dstDefUseNode, extTable, nodeMap, rdStore, ptrAnalysis);
					}
					else
					{
						auto& summary = summaryMap.getSummary(callee);
						for (auto loc: summary.mem_reads())
						{
							auto nodeSet = rdStore.getReachingDefs(loc);
							assert(nodeSet != nullptr);
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

	ReachingDefAnalysis rdAnalysis(ptrAnalysis, summaryMap, extTable);
	auto rdMap = rdAnalysis.runOnPointerCFG(cfg);

	// Set the entry and the exit
	auto entryDefUseNode = getDefUseNode(nodeMap, cfg.getEntryNode());
	auto exitDefUseNode = getDefUseNode(nodeMap, cfg.getExitNode());
	dug.setEntryNode(cast<EntryDefUseNode>(entryDefUseNode));
	dug.setExitNode(cast<ReturnDefUseNode>(exitDefUseNode));
	// This edge exists because we want the analysis to always reach the exit node
	entryDefUseNode->insertTopLevelEdge(exitDefUseNode);

	buildTopLevelEdges(cfg, nodeMap);
	buildMemLevelEdges(cfg, nodeMap, summaryMap, rdMap, ptrAnalysis, extTable);
}

DefUseProgram DefUseProgramBuilder::buildDefUseProgram(const PointerProgram& prog)
{
	DefUseProgram duProg;

	for (auto const& cfg: prog)
	{
		// Create the def-use graph from PointerCFG
		auto dug = duProg.createDefUseGraphForFunction(cfg.getFunction());
		buildDefUseGraph(*dug, cfg);
	}

	return duProg;
}

}
