#include "PointerControlFlow//SemiSparseProgramBuilder.h"

#include <llvm/IR/Module.h>

using namespace llvm;

namespace tpa
{

void SemiSparseProgramBuilder::detachPreservingNodes(PointerCFG& cfg)
{
	auto detachNode = [] (PointerCFGNode* node)
	{
		auto preds = std::vector<PointerCFGNode*>(node->pred_begin(), node->pred_end());
		for (auto const& predNode: preds)
		{
			// Ignore self-loop
			if (predNode == node)
				continue;

			for (auto const& succNode: node->succs())
			{
				// Again, ignore self-loop
				if (succNode == node)
					continue;

				predNode->insertEdge(succNode);
			}

			// Remove edges from predecessors
			predNode->removeEdge(node);
		}

		// Remove edges to successors
		auto succs = std::vector<PointerCFGNode*>(node->succ_begin(), node->succ_end());
		for (auto const& succNode: succs)
		{
			node->removeEdge(succNode);
		}
	};

	for (auto node: cfg)
	{
		if (isa<AllocNode>(node) || isa<CopyNode>(node))
			detachNode(node);
	}
}

PointerProgram SemiSparseProgramBuilder::buildSemiSparseProgram(const Module& module)
{
	auto prog = builder.buildPointerProgram(module);

	for (auto const& f: module)
	{
		auto cfg = prog.getPointerCFG(&f);
		if (cfg == nullptr)
			continue;

		detachPreservingNodes(*cfg);
	}

	return prog;
}

}