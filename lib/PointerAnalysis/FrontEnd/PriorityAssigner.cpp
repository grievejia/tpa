#include "PointerAnalysis/FrontEnd/CFG/PriorityAssigner.h"
#include "PointerAnalysis/Program/CFG/CFG.h"

namespace tpa
{

void PriorityAssigner::traverse()
{
	currLabel = 1;
	for (auto node: cfg)
		if (node->getPriority() == 0u)
			visitNode(node);
}

void PriorityAssigner::visitNode(tpa::CFGNode* node)
{
	if (!visitedNodes.insert(node).second)
		return;

	for (auto const& succ: node->succs())
		visitNode(succ);

	node->setPriority(currLabel);
	++currLabel;
}

}