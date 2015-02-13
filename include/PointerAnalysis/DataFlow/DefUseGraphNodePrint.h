#ifndef TPA_DEF_USE_GRAPH_NODE_PRINT_H
#define TPA_DEF_USE_GRAPH_NODE_PRINT_H

#include "PointerAnalysis/ControlFlow/NodePrint.h"
#include "PointerAnalysis/DataFlow/DefUseGraph.h"

namespace tpa
{

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const DefUseGraphNode& node)
{
	switch (node.getType())
	{
		case PointerCFGNodeType::Entry:
			os << llvm::cast<EntryDefUseNode>(node);
			break;
		case PointerCFGNodeType::Alloc:
			os << llvm::cast<AllocDefUseNode>(node);
			break;
		case PointerCFGNodeType::Copy:
			os << llvm::cast<CopyDefUseNode>(node);
			break;
		case PointerCFGNodeType::Load:
			os << llvm::cast<LoadDefUseNode>(node);
			break;
		case PointerCFGNodeType::Store:
			os << llvm::cast<StoreDefUseNode>(node);
			break;
		case PointerCFGNodeType::Call:
			os << llvm::cast<CallDefUseNode>(node);
			break;
		case PointerCFGNodeType::Ret:
			os << llvm::cast<ReturnDefUseNode>(node);
			break;
	}
	return os;
}

}

#endif
