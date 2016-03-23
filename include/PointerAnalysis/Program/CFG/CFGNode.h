#pragma once

#include "PointerAnalysis/Program/CFG/NodeMixin.h"
#include "Util/Iterator/IteratorRange.h"
#include "Util/DataStructure/VectorSet.h"

namespace llvm
{
	class Function;
}

namespace tpa
{

class CFG;

// Nodes in the pointer CFG
class CFGNode
{
private:
	CFGNodeTag tag;

	// The cfg that contains this node
	const CFG* cfg;

	// The reverse postorder number of this node
	size_t rpo;

	using NodeSet = util::VectorSet<CFGNode*>;
	// CFG edges
	NodeSet pred, succ;
	// Top-level def-use edges
	NodeSet def, use;
protected:
	CFGNode(CFGNodeTag t): tag(t), cfg(nullptr), rpo(0) {}

	void setCFG(const CFG& c) { cfg = &c; }
public:
	using iterator = NodeSet::iterator;
	using const_iterator = NodeSet::const_iterator;

	CFGNode(const CFGNode&) = delete;
	CFGNode(CFGNode&&) noexcept = default;
	CFGNode& operator=(const CFGNode&) = delete;
	CFGNode& operator=(CFGNode&&) = delete;

	CFGNodeTag getNodeTag() const { return tag; }
	bool isEntryNode() const { return tag == CFGNodeTag::Entry; }
	bool isAllocNode() const { return tag == CFGNodeTag::Alloc; }
	bool isCopyNode() const { return tag == CFGNodeTag::Copy; }
	bool isOffsetNode() const { return tag == CFGNodeTag::Offset; }
	bool isLoadNode() const { return tag == CFGNodeTag::Load; }
	bool isStoreNode() const { return tag == CFGNodeTag::Store; }
	bool isCallNode() const { return tag == CFGNodeTag::Call; }
	bool isReturnNode() const { return tag == CFGNodeTag::Ret; }

	const CFG& getCFG() const
	{
		assert(cfg != nullptr);
		return *cfg;
	}
	const llvm::Function& getFunction() const;

	size_t getPriority() const
	{
		return rpo;
	}
	void setPriority(size_t p)
	{
		assert(rpo == 0);
		rpo = p;
	}

	const_iterator pred_begin() const { return pred.begin(); }
	const_iterator pred_end() const { return pred.end(); }
	auto preds() const { return util::iteratorRange(pred.begin(), pred.end()); }
	unsigned pred_size() const { return pred.size(); }

	iterator succ_begin() { return succ.begin(); }
	iterator succ_end() { return succ.end(); }
	const_iterator succ_begin() const { return succ.begin(); }
	const_iterator succ_end() const { return succ.end(); }
	auto succs() const { return util::iteratorRange(succ.begin(), succ.end()); }
	unsigned succ_size() const { return succ.size(); }

	const_iterator def_begin() const { return def.begin(); }
	const_iterator def_end() const { return def.end(); }
	auto defs() const { return util::iteratorRange(def.begin(), def.end()); }
	unsigned def_size() const { return def.size(); }

	const_iterator use_begin() const { return use.begin(); }
	const_iterator use_end() const { return use.end(); }
	auto uses() const { return util::iteratorRange(use.begin(), use.end()); }
	unsigned use_size() const { return use.size(); }

	bool hasSuccessor(const CFGNode* node) const
	{
		return succ.count(const_cast<CFGNode*>(node));
	}
	bool hasUse(const CFGNode* node) const
	{
		return use.count(const_cast<CFGNode*>(node));
	}

	// Edge modifiers
	void insertEdge(CFGNode* n);
	void removeEdge(CFGNode* n);
	void insertDefUseEdge(CFGNode* n);
	void removeDefUseEdge(CFGNode* n);
	void detachFromCFG();

	friend class CFG;
};

using EntryCFGNode = EntryNodeMixin<CFGNode>;
using AllocCFGNode = AllocNodeMixin<CFGNode>;
using CopyCFGNode = CopyNodeMixin<CFGNode>;
using OffsetCFGNode = OffsetNodeMixin<CFGNode>;
using LoadCFGNode = LoadNodeMixin<CFGNode>;
using StoreCFGNode = StoreNodeMixin<CFGNode>;
using CallCFGNode = CallNodeMixin<CFGNode>;
using ReturnCFGNode = ReturnNodeMixin<CFGNode>;

}