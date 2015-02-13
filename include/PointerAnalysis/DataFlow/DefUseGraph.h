#ifndef TPA_DEF_USE_GRAPH_H
#define TPA_DEF_USE_GRAPH_H

#include "PointerAnalysis/ControlFlow/PointerCFG.h"

#include <unordered_map>

namespace tpa
{

class MemoryLocation;

class DefUseGraphNode
{
private:
	PointerCFGNodeType type;

	// The LLVM instruction that corresponds to this node
	const llvm::Instruction* inst;

	// The reverse postorder number of this node
	size_t rpo;

	using NodeSet = VectorSet<DefUseGraphNode*>;
	NodeSet topSucc;

	using NodeMap = std::unordered_map<const MemoryLocation*, NodeSet>;
	NodeMap memSucc;
protected:
	DefUseGraphNode(PointerCFGNodeType t, const llvm::Instruction* i): type(t), inst(i), rpo(0) {}
public:
	using iterator = NodeSet::iterator;
	using const_iterator = NodeSet::const_iterator;
	using mem_succ_iterator = NodeMap::iterator;
	using const_mem_succ_iterator = NodeMap::const_iterator;

	virtual ~DefUseGraphNode() {}

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

	bool insertTopLevelEdge(DefUseGraphNode* node);
	void insertMemLevelEdge(const MemoryLocation* loc, DefUseGraphNode* node);

	llvm::iterator_range<iterator> top_succs()
	{
		return llvm::iterator_range<iterator>(topSucc.begin(), topSucc.end());
	}
	llvm::iterator_range<const_iterator> top_succs() const
	{
		return llvm::iterator_range<const_iterator>(topSucc.begin(), topSucc.end());
	}
	llvm::iterator_range<mem_succ_iterator> mem_succs()
	{
		return llvm::iterator_range<mem_succ_iterator>(memSucc.begin(), memSucc.end());
	}
	llvm::iterator_range<const_mem_succ_iterator> mem_succs() const
	{
		return llvm::iterator_range<const_mem_succ_iterator>(memSucc.begin(), memSucc.end());
	}
	llvm::iterator_range<iterator> mem_succs(const MemoryLocation* loc)
	{
		auto itr = memSucc.find(loc);
		if (itr == memSucc.end())
			return llvm::iterator_range<iterator>(iterator(), iterator());
		else
			return llvm::iterator_range<iterator>(itr->second.begin(), itr->second.end());
	}
	llvm::iterator_range<const_iterator> mem_succs(const MemoryLocation* loc) const
	{
		auto itr = memSucc.find(loc);
		if (itr == memSucc.end())
			return llvm::iterator_range<const_iterator>(iterator(), iterator());
		else
			return llvm::iterator_range<const_iterator>(itr->second.begin(), itr->second.end());
	}
};

using EntryDefUseNode = EntryNodeMixin<DefUseGraphNode>;
using AllocDefUseNode = AllocNodeMixin<DefUseGraphNode>;
using CopyDefUseNode = CopyNodeMixin<DefUseGraphNode>;
using LoadDefUseNode = LoadNodeMixin<DefUseGraphNode>;
using StoreDefUseNode = StoreNodeMixin<DefUseGraphNode>;
using CallDefUseNode = CallNodeMixin<DefUseGraphNode>;
using ReturnDefUseNode = ReturnNodeMixin<DefUseGraphNode>;

class DefUseGraph
{
private:
	// Function
	const llvm::Function* function;

	// Nodes
	std::vector<std::unique_ptr<DefUseGraphNode>> nodes;

	// Instruction -> node mapping
	llvm::DenseMap<const llvm::Instruction*, DefUseGraphNode*> instToNode;

	// Entry and exit
	const EntryDefUseNode* entryNode;
	const ReturnDefUseNode* exitNode;

	void registerNode(DefUseGraphNode* node)
	{
		auto inst = node->getInstruction();
		if (inst != nullptr)
		{
			assert(!instToNode.count(inst) && "Trying to map one inst to two different nodes!");
			instToNode[inst] = node;
		}
	}
public:
	using const_param_iterator = llvm::Function::const_arg_iterator;
	using iterator = UniquePtrIterator<decltype(nodes)::iterator>;
	using const_iterator = UniquePtrIterator<decltype(nodes)::const_iterator>;

	explicit DefUseGraph(const llvm::Function* f): function(f), entryNode(nullptr), exitNode(nullptr) {}

	// Getters 
	const llvm::Function* getFunction() const { return function; }
	const EntryDefUseNode* getEntryNode() const { return entryNode; }
	const ReturnDefUseNode* getExitNode() const
	{
		assert(exitNode != nullptr);
		return exitNode;
	}
	DefUseGraphNode* getNodeFromInstruction(const llvm::Instruction* inst) const
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

	// Setters
	void setEntryNode(const EntryDefUseNode* node)
	{
		assert(entryNode == nullptr);
		entryNode = node;
	}
	void setExitNode(const ReturnDefUseNode* node)
	{
		assert(exitNode == nullptr);
		exitNode = node;
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

	void writeDotFile(const llvm::StringRef& filePath) const;

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

#endif
