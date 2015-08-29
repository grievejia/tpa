#pragma once

#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "Util/DataStructure/VectorSet.h"
#include "Util/Iterator/UniquePtrIterator.h"

#include <llvm/ADT/DenseMap.h>

#include <memory>
#include <vector>

namespace llvm
{
	class Function;
}

namespace tpa
{

class CFG
{
private:
	// Function
	const llvm::Function& func;

	// Nodes
	using NodeList = std::vector<std::unique_ptr<CFGNode>>;
	NodeList nodes;

	// Value to node mapping
	using ValueMap = llvm::DenseMap<const llvm::Value*, const CFGNode*>;
	ValueMap valueMap;

	// Entry and exit
	EntryCFGNode* entryNode;
	const ReturnCFGNode* exitNode;
public:
	using iterator = util::UniquePtrIterator<NodeList::iterator>;
	using const_iterator = util::UniquePtrConstIterator<NodeList::const_iterator>;

	CFG(const llvm::Function&);

	const llvm::Function& getFunction() const { return func; }
	EntryCFGNode* getEntryNode() { return entryNode; }
	const EntryCFGNode* getEntryNode() const { return entryNode; }
	
	bool doesNotReturn() const
	{
		return (exitNode == nullptr);
	}
	const ReturnCFGNode* getExitNode() const
	{
		assert(!doesNotReturn());
		return exitNode;
	}
	void setExitNode(const ReturnCFGNode* n)
	{
		assert(exitNode == nullptr && n != nullptr);
		exitNode = n;
	}

	void removeNodes(const util::VectorSet<CFGNode*>&);
	void buildValueMap();

	const CFGNode* getCFGNodeForValue(const llvm::Value* val) const
	{
		auto itr = valueMap.find(val);
		if (itr != valueMap.end())
			return itr->second;
		else
			return nullptr;
	}

	// Node creation
	template <typename Node, typename... Args>
	Node* create(Args&&... args)
	{
		// I can't use make_unique() here because the constructor for Node should be private
		auto node = new Node(std::forward<Args>(args)...);
		node->setCFG(*this);
		nodes.emplace_back(node);
		return node;
	}

	iterator begin() { return iterator(nodes.begin()); }
	iterator end() { return iterator(nodes.end()); }
	const_iterator begin() const { return const_iterator(nodes.begin()); }
	const_iterator end() const { return const_iterator(nodes.end()); }
	size_t getNumNodes() const { return nodes.size(); }
};

}