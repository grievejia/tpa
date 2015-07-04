#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"
#include "Util/IO/PointerAnalysis/NodePrinter.h"

#include <llvm/Support/raw_ostream.h>

namespace util
{
namespace io
{

template <typename T>
NodePrinter<T>::NodePrinter(llvm::raw_ostream& o): os(o) {}

template <typename T>
void NodePrinter<T>::visitEntryNode(const tpa::EntryNodeMixin<T>&)
{
	os << "[ENTRY]";
}

template <typename T>
void NodePrinter<T>::visitAllocNode(const tpa::AllocNodeMixin<T>& node)
{
	auto allocType = node.getAllocTypeLayout();
	os << "[ALLOC] " << node.getDest()->getName() << " (size = " << allocType->getSize() << ")";
}

template <typename T>
void NodePrinter<T>::visitReturnNode(const tpa::ReturnNodeMixin<T>& node)
{
	os << "[RET] return ";
	auto ret = node.getReturnValue();
	if (ret != nullptr)
		os << ret->getName();
}

template <typename T>
void NodePrinter<T>::visitCopyNode(const tpa::CopyNodeMixin<T>& node)
{
	os << "[COPY] " << node.getDest()->getName() << " = ";
	for (auto const& src: node)
			os << src->getName() << " ";
}

template <typename T>
void NodePrinter<T>::visitOffsetNode(const tpa::OffsetNodeMixin<T>& node)
{
	os << "[OFFSET] " << node.getDest()->getName() << " = " << node.getSrc()->getName() << " + " << node.getOffset();
	if (node.isArrayRef())
		os << " []";
}

template <typename T>
void NodePrinter<T>::visitLoadNode(const tpa::LoadNodeMixin<T>& node)
{
	os << "[LOAD] " << node.getDest()->getName() << " = *" << node.getSrc()->getName();
}

template <typename T>
void NodePrinter<T>::visitStoreNode(const tpa::StoreNodeMixin<T>& node)
{
	os << "[STORE] *" << node.getDest()->getName() << " = " << node.getSrc()->getName();
}

template <typename T>
void NodePrinter<T>::visitCallNode(const tpa::CallNodeMixin<T>& node)
{
	os << "[CALL] ";
	if (node.getDest() != nullptr)
		os << node.getDest()->getName() << " = ";

	os << node.getFunctionPointer()->getName() << " ( ";
	for (auto const& arg: node)
		os << arg->getName() << " ";
	os << ")";
}

// Explicit instantiation
template class NodePrinter<tpa::CFGNode>;

}
}