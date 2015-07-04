#include "PointerAnalysis/Program/CFG/CFG.h"

#include <llvm/ADT/SmallVector.h>

namespace tpa
{

const llvm::Function& CFGNode::getFunction() const
{
	assert(cfg != nullptr);
	return cfg->getFunction();
}

void CFGNode::insertEdge(CFGNode* node)
{
	assert(node != nullptr);
	
	succ.insert(node);
	(node->pred).insert(this);
}

void CFGNode::removeEdge(CFGNode* node)
{
	assert(node != nullptr);

	succ.erase(node);
	(node->pred).erase(this);
}

void CFGNode::insertDefUseEdge(CFGNode* node)
{
	assert(node != nullptr);

	use.insert(node);
	node->def.insert(this);
}

void CFGNode::removeDefUseEdge(CFGNode* node)
{
	assert(node != nullptr);

	use.erase(node);
	node->def.erase(this);
}

void CFGNode::detachFromCFG()
{
	// Remove edges to predecessors
	auto preds = llvm::SmallVector<CFGNode*, 8>(pred.begin(), pred.end());
	for (auto predNode: preds)
	{
		// Ignore self-loop
		if (predNode == this)
			continue;

		for (auto succNode: succ)
		{
			// Again, ignore self-loop
			if (succNode == this)
				continue;

			predNode->insertEdge(succNode);
		}

		// Remove edges from predecessors
		predNode->removeEdge(this);
	}

	// Remove edges to successors
	auto succs = llvm::SmallVector<CFGNode*, 8>(succ.begin(), succ.end());
	for (auto succNode: succs)
		removeEdge(succNode);
}

CFG::CFG(const llvm::Function& f): func(f), entryNode(create<EntryCFGNode>()), exitNode(nullptr)
{
	entryNode->setCFG(*this);
}

void CFG::removeNodes(const util::VectorSet<CFGNode*>& removeSet)
{
	if (removeSet.empty())
		return;
	
	NodeList newNodeList;
	newNodeList.reserve(nodes.size());
	for (auto& node: nodes)
	{
		if (!removeSet.count(node.get()))
			newNodeList.emplace_back(std::move(node));
		else
		{
			node->detachFromCFG();
			assert(node->pred_size() == 0u && node->succ_size() == 0u && node->def_size() == 0u && node->use_size() == 0u);
		}
	}
	nodes.swap(newNodeList);
}

}