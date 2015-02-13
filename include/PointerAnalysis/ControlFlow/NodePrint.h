#ifndef TPA_NODE_PRINT_H
#define TPA_NODE_PRINT_H

#include "NodeMixins.h"

#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

namespace tpa
{

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const EntryNodeMixin<T>& node)
{
	os << "[ENTRY]";
	return os;
}

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const AllocNodeMixin<T>& node)
{
	os << "[ALLOC] " << node.getDest()->getName() << " :: ";
	auto allocType = node.getAllocType();
	if (allocType->isStructTy() && llvm::cast<llvm::StructType>(allocType)->hasName())
		os << allocType->getStructName();
	else
		os << *allocType;
	return os;
}

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const ReturnNodeMixin<T>& node)
{
	os << "[RET] return ";
	auto ret = node.getReturnValue();
	if (ret != nullptr)
	{
		if (llvm::isa<llvm::ConstantPointerNull>(ret))
			os << "null";
		else
			os << ret->getName();
	}
	return os;
}

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const CopyNodeMixin<T>& node)
{
	os << "[COPY] " << node.getDest()->getName() << " = ";
	auto offset = node.getOffset();
	if (offset > 0)
	{
		os << node.getFirstSrc()->getName() << " + " << offset;
		if (node.isArrayRef())
			os << " []";
	}
	else
	{
		for (auto const& src: node)
			os << src->getName() << " ";
	}
	return os;
}

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const LoadNodeMixin<T>& node)
{
	os << "[LOAD] " << node.getDest()->getName() << " = *" << node.getSrc()->getName();
	return os;
}

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const StoreNodeMixin<T>& node)
{
	os << "[STORE] *" << node.getDest()->getName() << " = " << node.getSrc()->getName();
	return os;
}

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const CallNodeMixin<T>& node)
{
	os << "[CALL] ";
	if (node.getDest() != nullptr)
		os << node.getDest()->getName() << " = ";

	os << node.getFunctionPointer()->getName() << " ( ";
	for (auto const& arg: node)
		os << arg->getName() << " ";
	os << ")";
	return os;
}

}

#endif
