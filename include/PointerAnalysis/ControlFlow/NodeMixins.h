#ifndef TPA_NODE_MIXINS_H
#define TPA_NODE_MIXINS_H

#include <llvm/IR/Instructions.h>

namespace tpa
{

enum class PointerCFGNodeType: std::uint8_t
{
	Entry,
	Alloc,
	Copy,
	Load,
	Store,
	Call,
	Ret,
};

template <typename NodeType>
class EntryNodeMixin: public NodeType
{
private:
	const llvm::Function* parent;
public:
	EntryNodeMixin(const llvm::Function* f): NodeType(PointerCFGNodeType::Entry, nullptr), parent(f) {}

	const llvm::Function* getFunction() const { return parent; }

	// Interface for LLVM-style rtti
	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Entry;
	}
};

template <typename NodeType>
class AllocNodeMixin: public NodeType
{
public:
	AllocNodeMixin(const llvm::Instruction* i): NodeType(PointerCFGNodeType::Alloc, i) {}

	const llvm::Value* getDest() const { return this->getInstruction(); }
	llvm::Type* getAllocType() const
	{
		return llvm::cast<llvm::AllocaInst>(this->getInstruction())->getAllocatedType();
	}

	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Alloc;
	}
};

template <typename NodeType>
class CopyNodeMixin: public NodeType
{
private:
	using SrcSet = std::vector<const llvm::Value*>;

	SrcSet srcs;
	size_t offset;
	bool arrayRef;
public:
	// Single-source copy
	CopyNodeMixin(const llvm::Instruction* i, const llvm::Value* s, size_t o, bool r): NodeType(PointerCFGNodeType::Copy, i), offset(o), arrayRef(r)
	{
		srcs.push_back(s);
	}
	// Multi-source copy (with offset forced to 0)
	CopyNodeMixin(const llvm::Instruction* i): NodeType(PointerCFGNodeType::Copy, i), offset(0), arrayRef(false) {}

	using const_iterator = SrcSet::const_iterator;

	const llvm::Value* getDest() const { return this->getInstruction(); }
	const llvm::Value* getFirstSrc() const 
	{
		assert(!srcs.empty() && "Getting the front of an empty copy-list!");
		return srcs.front();
	}
	int getOffset() const { return offset; }
	bool isArrayRef() const { return arrayRef; }

	void addSrc(const llvm::Value* v) { srcs.push_back(v); }
	void removeSrc(const llvm::Value* v)
	{
		srcs.erase(std::remove(srcs.begin(), srcs.end(), v), srcs.end());
	}
	const llvm::Value* getSrc(unsigned idx) const { return srcs.at(idx); }
	unsigned getNumSrc() const { return srcs.size(); }

	const_iterator begin() const { return srcs.begin(); }
	const_iterator end() const { return srcs.end(); }

	// Interface for LLVM-style rtti
	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Copy;
	}
};

template <typename NodeType>
class LoadNodeMixin: public NodeType
{
private:
	const llvm::Value* src;
public:
	LoadNodeMixin(const llvm::Instruction* i, const llvm::Value* s): NodeType(PointerCFGNodeType::Load, i), src(s) {}

	const llvm::Value* getDest() const { return this->getInstruction(); }
	const llvm::Value* getSrc() const { return src; }

	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Load;
	}
};

template <typename NodeType>
class StoreNodeMixin: public NodeType
{
private:
	const llvm::Value* dest;
	const llvm::Value* src;
public:
	StoreNodeMixin(const llvm::Instruction* i, const llvm::Value* d, const llvm::Value* s): NodeType(PointerCFGNodeType::Store, i), dest(d), src(s) {}

	const llvm::Value* getDest() const { return dest; }
	const llvm::Value* getSrc() const { return src; }

	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Store;
	}
};

template <typename NodeType>
class CallNodeMixin: public NodeType
{
private:
	using ArgumentSet = std::vector<const llvm::Value*>;

	// dest can be NULL here if the call returns void type
	const llvm::Value* dest;
	const llvm::Value* funPtr;
	ArgumentSet args;
public:
	CallNodeMixin(const llvm::Instruction* i, const llvm::Value* f, const llvm::Value* d = nullptr): NodeType(PointerCFGNodeType::Call, i), dest(d), funPtr(f) {}

	using const_iterator = ArgumentSet::const_iterator;

	const llvm::Value* getDest() const { return dest; }
	const llvm::Value* getFunctionPointer() const { return funPtr; }

	void addArgument(const llvm::Value* v) { args.push_back(v); }
	const llvm::Value* getArgument(unsigned idx) const { return args.at(idx); }
	unsigned getNumArgument() const { return args.size(); }

	const_iterator begin() const { return args.begin(); }
	const_iterator end() const { return args.end(); }

	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Call;
	}
};

template <typename NodeType>
class ReturnNodeMixin: public NodeType
{
private:
	// ret can be NULL if the function returns a void type
	const llvm::Value* ret;
public:
	ReturnNodeMixin(const llvm::Instruction* i, const llvm::Value* r): NodeType(PointerCFGNodeType::Ret, i), ret(r) {}

	const llvm::Value* getReturnValue() const { return ret; }

	static bool classof(const NodeType* n)
	{
		return n->getType() == PointerCFGNodeType::Ret;
	}
};

}

#endif
