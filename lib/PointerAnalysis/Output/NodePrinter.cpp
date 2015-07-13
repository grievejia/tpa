#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"
#include "Util/IO/PointerAnalysis/NodePrinter.h"

#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

namespace util
{
namespace io
{

static void dumpValue(llvm::raw_ostream& os, const llvm::Value* v)
{
	if (llvm::isa<llvm::UndefValue>(v))
		os << "(anywhere)";
	else if (llvm::isa<llvm::ConstantPointerNull>(v))
		os << "(null)";
	else
		os << v->getName();
}

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
		dumpValue(os, ret);
}

template <typename T>
void NodePrinter<T>::visitCopyNode(const tpa::CopyNodeMixin<T>& node)
{
	os << "[COPY] " << node.getDest()->getName() << " = ";
	for (auto const& src: node)
	{
		dumpValue(os, src);
		os << " ";
	}
}

template <typename T>
void NodePrinter<T>::visitOffsetNode(const tpa::OffsetNodeMixin<T>& node)
{
	os << "[OFFSET] ";
	dumpValue(os, node.getDest());
	os << " = ";
	dumpValue(os, node.getSrc());
	os << " + " << node.getOffset();
	if (node.isArrayRef())
		os << " []";
}

template <typename T>
void NodePrinter<T>::visitLoadNode(const tpa::LoadNodeMixin<T>& node)
{
	os << "[LOAD] ";
	dumpValue(os, node.getDest());
	os << " = *";
	dumpValue(os, node.getSrc());
}

template <typename T>
void NodePrinter<T>::visitStoreNode(const tpa::StoreNodeMixin<T>& node)
{
	os << "[STORE] *";
	dumpValue(os, node.getDest());
	os << " = ";
	dumpValue(os, node.getSrc());
}

template <typename T>
void NodePrinter<T>::visitCallNode(const tpa::CallNodeMixin<T>& node)
{
	os << "[CALL] ";
	if (node.getDest() != nullptr)
		os << node.getDest()->getName() << " = ";

	os << node.getFunctionPointer()->getName() << " ( ";
	for (auto const& arg: node)
	{
		dumpValue(os, arg);
		os << " ";
	}
	os << ")";
}

// Explicit instantiation
template class NodePrinter<tpa::CFGNode>;

}
}