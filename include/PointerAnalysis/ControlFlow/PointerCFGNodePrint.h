#ifndef TPA_POINTER_CFG_NODE_PRINT_H
#define TPA_POINTER_CFG_NODE_PRINT_H

#include "PointerAnalysis/ControlFlow/NodePrint.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"

namespace tpa
{

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const PointerCFGNode& node)
{
	switch (node.getType())
	{
		case PointerCFGNodeType::Entry:
			os << llvm::cast<EntryNode>(node);
			break;
		case PointerCFGNodeType::Alloc:
			os << llvm::cast<AllocNode>(node);
			break;
		case PointerCFGNodeType::Copy:
			os << llvm::cast<CopyNode>(node);
			break;
		case PointerCFGNodeType::Load:
			os << llvm::cast<LoadNode>(node);
			break;
		case PointerCFGNodeType::Store:
			os << llvm::cast<StoreNode>(node);
			break;
		case PointerCFGNodeType::Call:
			os << llvm::cast<CallNode>(node);
			break;
		case PointerCFGNodeType::Ret:
			os << llvm::cast<ReturnNode>(node);
			break;
	}
	return os;
}

}

#endif
