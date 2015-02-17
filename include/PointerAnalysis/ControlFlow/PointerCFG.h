#ifndef TPA_POINTER_CFG_H
#define TPA_POINTER_CFG_H

#include "PointerAnalysis/ControlFlow/NodeMixins.h"
#include "Utils/UniquePtrIterator.h"
#include "Utils/VectorSet.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

namespace tpa
{

// Nodes in the pointer CFG
// Note that the constructors and destructors of this class and its subclasses are declared private. This is intentional because we don't want other entity except PointerCFG objects to create and destroy the nodes
class PointerCFGNode
{
private:
	PointerCFGNodeType type;

	// The LLVM instruction that corresponds to this node
	const llvm::Instruction* inst;

	// The reverse postorder number of this node
	// 0 = unset
	size_t rpo;

	using NodeSet = VectorSet<PointerCFGNode*>;
	// CFG edges
	NodeSet pred, succ;
	// Top-level def-use edges
	NodeSet def, use;
protected:
	PointerCFGNode(PointerCFGNodeType t, const llvm::Instruction* i): type(t), inst(i), rpo(0) {}
public:
	using iterator = NodeSet::iterator;
	using const_iterator = NodeSet::const_iterator;

	// The destructor must be virtual here because we want to use PointerCFGNode* polymorphically.
	virtual ~PointerCFGNode() {}

	PointerCFGNodeType getType() const { return type; }
	const llvm::Instruction* getInstruction() const { return inst; }
	const llvm::Function* getFunction() const
	{
		assert(inst != nullptr);
		return inst->getParent()->getParent();
	}

	size_t getPriority() const
	{
		assert(rpo != 0);
		return rpo;
	}
	bool hasPriority() const { return rpo != 0; }
	void setPriority(size_t o)
	{
		assert(o != 0 && "0 cannot be used as a priority num!");
		rpo = o;
	}

	const_iterator pred_begin() const { return pred.begin(); }
	const_iterator pred_end() const { return pred.end(); }
	llvm::iterator_range<const_iterator> preds() const
	{
		return llvm::iterator_range<const_iterator>(pred_begin(), pred_end());
	}
	unsigned pred_getSize() const { return pred.getSize(); }

	iterator succ_begin() { return succ.begin(); }
	iterator succ_end() { return succ.end(); }
	const_iterator succ_begin() const { return succ.begin(); }
	const_iterator succ_end() const { return succ.end(); }
	llvm::iterator_range<const_iterator> succs() const
	{
		return llvm::iterator_range<const_iterator>(succ_begin(), succ_end());
	}
	unsigned succ_getSize() const { return succ.getSize(); }

	const_iterator def_begin() const { return def.begin(); }
	const_iterator def_end() const { return def.end(); }
	llvm::iterator_range<const_iterator> defs() const
	{
		return llvm::iterator_range<const_iterator>(def_begin(), def_end());
	}
	unsigned def_getSize() const { return def.getSize(); }

	const_iterator use_begin() const { return use.begin(); }
	const_iterator use_end() const { return use.end(); }
	llvm::iterator_range<const_iterator> uses() const
	{
		return llvm::iterator_range<const_iterator>(use_begin(), use_end());
	}
	unsigned use_getSize() const { return use.getSize(); }

	bool hasSuccessor(const PointerCFGNode* node) const
	{
		return succ.has(const_cast<PointerCFGNode*>(node));
	}
	bool hasUse(const PointerCFGNode* node) const
	{
		return use.has(const_cast<PointerCFGNode*>(node));
	}

	// Edge modifiers
	void insertEdge(PointerCFGNode* n);
	void removeEdge(PointerCFGNode* n);
	void insertDefUseEdge(PointerCFGNode* n);
	void removeDefUseEdge(PointerCFGNode* n);

	friend class PointerCFG;
};

// EntryNode is, well, the entry of a PointerCFG
using EntryNode = EntryNodeMixin<PointerCFGNode>;

// AllocNode represents a memory allocation operation
using AllocNode = AllocNodeMixin<PointerCFGNode>;

// CopyNode represent a pointer-copy or pointer-arithmetic operation
// It can have a single rhs and a non-zero offset, which means pointer arithmetic
// Or it can have single or multiple rhs and a zero offset, which means pointer copy
using CopyNode = CopyNodeMixin<PointerCFGNode>;

using LoadNode = LoadNodeMixin<PointerCFGNode>;
using StoreNode = StoreNodeMixin<PointerCFGNode>;

// CallNode represent an ordinary function call
// We won't try to distinguish direct and indirect function call here. Direct calls are translated into indirect calls with a single-destination pointer
using CallNode = CallNodeMixin<PointerCFGNode>;

using ReturnNode = ReturnNodeMixin<PointerCFGNode>;

// The CFG that only contains pointer-related operations

class PointerCFG
{
private:
	// Function
	const llvm::Function* function;

	// Nodes
	std::vector<std::unique_ptr<PointerCFGNode>> nodes;

	// Instruction -> node mapping
	llvm::DenseMap<const llvm::Instruction*, PointerCFGNode*> instToNode;

	// Entry and exit
	EntryNode* entryNode;
	const ReturnNode* exitNode;

	// Helper function to create/delete a node
	void registerNode(PointerCFGNode* node)
	{
		auto inst = node->getInstruction();
		if (inst != nullptr)
		{
			assert(!instToNode.count(inst) && "Trying to map one inst to two different nodes!");
			instToNode[inst] = node;
		}
	}
	void unregisterNode(PointerCFGNode* node)
	{
		if (auto inst = node->getInstruction())
			instToNode.erase(inst);
	}
public:
	using NodeType = PointerCFGNode;

	using const_param_iterator = llvm::Function::const_arg_iterator;
	using iterator = UniquePtrIterator<decltype(nodes)::iterator>;
	using const_iterator = UniquePtrIterator<decltype(nodes)::const_iterator>;

	explicit PointerCFG(const llvm::Function* f);

	// Getters 
	const llvm::Function* getFunction() const { return function; }
	EntryNode* getEntryNode() { return entryNode; }
	const EntryNode* getEntryNode() const { return entryNode; }
	const ReturnNode* getExitNode() const
	{
		assert(exitNode != nullptr);
		return exitNode;
	}
	PointerCFGNode* getNodeFromInstruction(const llvm::Instruction* inst) const
	{
		auto itr = instToNode.find(inst);
		if (itr == instToNode.end())
			return nullptr;
		else
			return itr->second;
	}
	size_t getNumNodes() const
	{
		return nodes.size();
	}

	void setExitNode(const ReturnNode* n)
	{
		assert(exitNode == nullptr);
		exitNode = n;
	}

	// Node creation
	template <typename Node, typename... Args>
	Node* create(Args&&... args)
	{
		// I can't use make_unique() here because the constructor for Node should be private
		auto node = new Node(std::forward<Args>(args)...);
		nodes.emplace_back(node);
		//auto node = static_cast<Node*>(nodes.back().get());
		registerNode(node);
		return node;
	}

	// Dump .dot file into the /dots subdir of current directory
	void writeDotFile(const llvm::StringRef& filePath) const;
	void writeDefUseDotFile(const llvm::StringRef& filePath) const;

	// Iterators
	const_param_iterator param_begin() const { return function->arg_begin(); }
	const_param_iterator param_end() const { return function->arg_end(); }
	llvm::iterator_range<const_param_iterator> params() const
	{
		return llvm::iterator_range<const_param_iterator>(param_begin(), param_end());
	}
	iterator begin() { return iterator(nodes.begin()); }
	iterator end() { return iterator(nodes.end()); }
	const_iterator begin() const { return const_iterator(nodes.begin()); }
	const_iterator end() const { return const_iterator(nodes.end()); }
};

}

namespace llvm
{

// GraphTraits specialization of PointerCFG
template <> struct GraphTraits<tpa::PointerCFG*>
{
	using NodeType = tpa::PointerCFGNode;
	static NodeType* getEntryNode(tpa::PointerCFG* cfg)
	{
		return cfg->getEntryNode();
	}

	using ChildIteratorType = NodeType::iterator;
	static ChildIteratorType child_begin(NodeType* node)
	{
		return node->succ_begin();
	}
	static ChildIteratorType child_end(NodeType* node)
	{
		return node->succ_end();
	}

	using nodes_iterator = tpa::PointerCFG::iterator;
	static nodes_iterator nodes_begin(tpa::PointerCFG* g)
	{
		return g->begin();
	}
	static nodes_iterator nodes_end(tpa::PointerCFG* g)
	{
		return g->end();
	}
	static unsigned size(tpa::PointerCFG* g)
	{
		return g->getNumNodes();
	}
};

template <> struct GraphTraits<const tpa::PointerCFG*>
{
	using NodeType = const tpa::PointerCFGNode;
	static NodeType* getEntryNode(const tpa::PointerCFG* cfg)
	{
		return cfg->getEntryNode();
	}
	
	using ChildIteratorType = tpa::PointerCFGNode::const_iterator;
	static ChildIteratorType child_begin(NodeType* node)
	{
		return node->succ_begin();
	}
	static ChildIteratorType child_end(NodeType* node)
	{
		return node->succ_end();
	}

	using nodes_iterator = tpa::PointerCFG::const_iterator;
	static nodes_iterator nodes_begin(const tpa::PointerCFG* g)
	{
		return g->begin();
	}
	static nodes_iterator nodes_end(const tpa::PointerCFG* g)
	{
		return g->end();
	}
	static unsigned size(const tpa::PointerCFG* g)
	{
		return g->getNumNodes();
	}
};

}

#endif
