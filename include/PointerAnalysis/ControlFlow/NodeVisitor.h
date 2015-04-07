#ifndef TPA_NODE_VISITOR_H
#define TPA_NODE_VISITOR_H

#include "PointerAnalysis/ControlFlow/NodeMixins.h"

namespace tpa
{

template <typename SubClass, typename RetType = void>
class NodeVisitor
{
private:
public:
	template <typename NodeType>
	RetType visit(const NodeType* node)
	{
		switch (node->getType())
		{
			case PointerCFGNodeType::Entry:
				return static_cast<SubClass*>(this)->visitEntryNode(static_cast<const EntryNodeMixin<NodeType>*>(node));
			case PointerCFGNodeType::Alloc:
				return static_cast<SubClass*>(this)->visitAllocNode(static_cast<const AllocNodeMixin<NodeType>*>(node));
			case PointerCFGNodeType::Copy:
				return static_cast<SubClass*>(this)->visitCopyNode(static_cast<const CopyNodeMixin<NodeType>*>(node));
			case PointerCFGNodeType::Load:
				return static_cast<SubClass*>(this)->visitLoadNode(static_cast<const LoadNodeMixin<NodeType>*>(node));
			case PointerCFGNodeType::Store:
				return static_cast<SubClass*>(this)->visitStoreNode(static_cast<const StoreNodeMixin<NodeType>*>(node));
			case PointerCFGNodeType::Call:
				return static_cast<SubClass*>(this)->visitCallNode(static_cast<const CallNodeMixin<NodeType>*>(node));
			case PointerCFGNodeType::Ret:
				return static_cast<SubClass*>(this)->visitReturnNode(static_cast<const ReturnNodeMixin<NodeType>*>(node));
		}
	}
};

}

#endif
