#include "PointerAnalysis/FrontEnd/CFG/FunctionTranslator.h"
#include "PointerAnalysis/FrontEnd/CFG/InstructionTranslator.h"
#include "PointerAnalysis/FrontEnd/CFG/PriorityAssigner.h"
#include "PointerAnalysis/Program/CFG/CFG.h"

#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

void FunctionTranslator::translateBasicBlock(const Function& llvmFunc)
{
	for (auto const& currBlock: llvmFunc)
	{
		tpa::CFGNode* startNode = nullptr;
		tpa::CFGNode* endNode = nullptr;

		for (auto const& inst: currBlock)
		{
			auto currNode = translator.visit(const_cast<Instruction&>(inst));
			if (currNode == nullptr)
				continue;
			else
			{
				instToNode[&inst] = currNode;
				nodeToInst[currNode] = &inst;
			}

			// Update the first node
			if (startNode == nullptr)
				startNode = currNode;
			// Chain the node with the last one
			if (endNode != nullptr)
				endNode->insertEdge(currNode);
			endNode = currNode;
		}

		assert((startNode == nullptr) == (endNode == nullptr));
		if (startNode != nullptr)
			bbToNode.insert(std::make_pair(&currBlock, std::make_pair(startNode, endNode)));
		else
			nonEmptySuccMap[&currBlock] = std::vector<tpa::CFGNode*>();
	}
}

// TODO: This function contains some legacy codes. Refactoring needed
void FunctionTranslator::processEmptyBlock()
{
	auto processedEmptyBlock = SmallPtrSet<const BasicBlock*, 128>();
	for (auto& mapping: nonEmptySuccMap)
	{
		auto currBlock = mapping.first;
		auto succs = SmallPtrSet<tpa::CFGNode*, 16>();

		// The following codes try to implement a non-recursive DFS for finding all reachable non-empty blocks starting from an empty block
		auto workList = std::vector<const BasicBlock*>();
		workList.insert(workList.end(), succ_begin(currBlock), succ_end(currBlock));
		auto visitedEmptyBlock = SmallPtrSet<const BasicBlock*, 16>();
		visitedEmptyBlock.insert(currBlock);

		while (!workList.empty())
		{
			auto nextBlock = workList.back();
			workList.pop_back();

			if (bbToNode.count(nextBlock))
			{
				succs.insert(bbToNode[nextBlock].first);
			}
			else if (processedEmptyBlock.count(nextBlock))
			{
				auto& nextVec = nonEmptySuccMap[nextBlock];
				succs.insert(nextVec.begin(), nextVec.end());
			}
			else
			{
				for (auto itr = succ_begin(nextBlock), ite = succ_end(nextBlock); itr != ite; ++itr)
				{
					auto nextNextBlock = *itr;
					if (visitedEmptyBlock.count(nextNextBlock))
						continue;
					if (!bbToNode.count(nextNextBlock))
						visitedEmptyBlock.insert(nextNextBlock);
					workList.push_back(nextNextBlock);
				}
			}
		}

		processedEmptyBlock.insert(currBlock);
		mapping.second.insert(mapping.second.end(), succs.begin(), succs.end());
	}
}

void FunctionTranslator::connectCFGNodes(const BasicBlock& entryBlock)
{
	for (auto& mapping: bbToNode)
	{
		auto bb = mapping.first;
		auto lastNode = mapping.second.second;

		for (auto itr = succ_begin(bb), ite = succ_end(bb); itr != ite; ++itr)
		{
			auto nextBB = *itr;
			auto bbItr = bbToNode.find(nextBB);
			if (bbItr != bbToNode.end())
				lastNode->insertEdge(bbItr->second.first);
			else
			{
				assert(nonEmptySuccMap.count(nextBB));
				auto& vec = nonEmptySuccMap[nextBB];
				for (auto succNode: vec)
					lastNode->insertEdge(succNode);
			}
		}
	}

	// Connect the entry node with the main graph
	if (bbToNode.count(&entryBlock))
		cfg.getEntryNode()->insertEdge(bbToNode[&entryBlock].first);
	else
	{
		assert(nonEmptySuccMap.count(&entryBlock));
		auto& vec = nonEmptySuccMap[&entryBlock];
		for (auto node: vec)
			cfg.getEntryNode()->insertEdge(node);
	}
}

void FunctionTranslator::drawDefUseEdgeFromValue(const Value* defVal, tpa::CFGNode* useNode)
{
	assert(defVal != nullptr && useNode != nullptr);

	if (!defVal->getType()->isPointerTy())
		return;

	if (isa<GlobalValue>(defVal) || isa<Argument>(defVal) || isa<UndefValue>(defVal) || isa<ConstantPointerNull>(defVal))
	{
		// Nodes that use global values are def roots
		cfg.getEntryNode()->insertDefUseEdge(useNode);
	}
	else if (auto defInst = dyn_cast<Instruction>(defVal))
	{
		// For instructions, see if we have corresponding node attached to it
		if (auto defNode = instToNode[defInst])
			defNode->insertDefUseEdge(useNode);
		else
			errs() << "Failed to find node for instruction " << *defInst << "\n";	
	}
}

void FunctionTranslator::constructDefUseChains()
{
	for (auto useNode: cfg)
	{
		switch (useNode->getNodeTag())
		{
			case CFGNodeTag::Entry:
				break;
			case CFGNodeTag::Alloc:
				cfg.getEntryNode()->insertDefUseEdge(useNode);
				break;
			case CFGNodeTag::Copy:
			{
				auto copyNode = static_cast<const CopyCFGNode*>(useNode);
				for (auto src: *copyNode)
				{
					auto defVal = src->stripPointerCasts();
					drawDefUseEdgeFromValue(defVal, useNode);
				}
				break;
			}
			case CFGNodeTag::Offset:
			{
				auto offsetNode = static_cast<const OffsetCFGNode*>(useNode);
				auto defVal = offsetNode->getSrc()->stripPointerCasts();
				drawDefUseEdgeFromValue(defVal, useNode);
				break;
			}
			case CFGNodeTag::Load:
			{
				auto loadNode = static_cast<const LoadCFGNode*>(useNode);
				auto defVal = loadNode->getSrc()->stripPointerCasts();
				drawDefUseEdgeFromValue(defVal, useNode);
				break;
			}
			case CFGNodeTag::Store:
			{
				auto storeNode = static_cast<const StoreCFGNode*>(useNode);
				auto srcVal = storeNode->getSrc()->stripPointerCasts();
				drawDefUseEdgeFromValue(srcVal, useNode);
				auto dstVal = storeNode->getDest()->stripPointerCasts();
				drawDefUseEdgeFromValue(dstVal, useNode);
				break;
			}
			case CFGNodeTag::Call:
			{
				auto callNode = static_cast<const CallCFGNode*>(useNode);
				auto funPtr = callNode->getFunctionPointer()->stripPointerCasts();
				drawDefUseEdgeFromValue(funPtr, useNode);
				for (auto arg: *callNode)
				{
					auto defVal = arg->stripPointerCasts();
					drawDefUseEdgeFromValue(defVal, useNode);
				}
				break;
			}
			case CFGNodeTag::Ret:
			{
				auto retNode = static_cast<const ReturnCFGNode*>(useNode);
				auto retVal = retNode->getReturnValue();
				if (retVal != nullptr)
				{
					auto defVal = retVal->stripPointerCasts();
					drawDefUseEdgeFromValue(defVal, useNode);
				}
				break;
			}
		}
	}
}

void FunctionTranslator::computeNodePriority()
{
	PriorityAssigner pa(cfg);
	pa.traverse();
}

void FunctionTranslator::detachStorePreservingNodes()
{
	for (auto node: cfg)
	{
		if (node->isAllocNode() || node->isCopyNode() || node->isOffsetNode())
			node->detachFromCFG();
	}
}

void FunctionTranslator::translateFunction(const Function& llvmFunc)
{
	// Scan the basic blocks and create the nodes first. We will worry about how to connect them later
	translateBasicBlock(llvmFunc);

	// Now the biggest problem are those "empty blocks" (i.e. blocks that do not contain any tpa::CFGNode). Those blocks may form cycles. So we need to know, in advance, what are the non empty successors of the empty blocks.
	processEmptyBlock();
	
	// Connect all the cfg nodes we've built
	connectCFGNodes(llvmFunc.getEntryBlock());

	// Draw def-use edges
	constructDefUseChains();

	// Compute the priority of each node
	computeNodePriority();

	// Detach all store-preserving nodes
	detachStorePreservingNodes();
}

}