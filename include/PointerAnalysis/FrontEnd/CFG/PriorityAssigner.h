#pragma once

#include <llvm/ADT/SmallPtrSet.h>

namespace tpa
{

class CFG;
class CFGNode;

class PriorityAssigner
{
private:
	CFG& cfg;
	size_t currLabel;

	using NodeSet = llvm::SmallPtrSet<const CFGNode*, 128>;
	NodeSet visitedNodes;

	void visitNode(CFGNode*);
public:
	PriorityAssigner(CFG& c): cfg(c), currLabel(1) {}

	void traverse();
};

}
