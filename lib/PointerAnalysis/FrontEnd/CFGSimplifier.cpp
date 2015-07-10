#include "PointerAnalysis/FrontEnd/CFG/CFGSimplifier.h"
#include "PointerAnalysis/Program/CFG/CFG.h"
#include "PointerAnalysis/Program/CFG/NodeVisitor.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>

using namespace llvm;

namespace tpa
{

namespace
{

class CFGAdjuster: public NodeVisitor<CFGAdjuster>
{
private:
	using MapType = DenseMap<const Value*, const Value*>;
	const MapType& eqMap;

	using SetType = util::VectorSet<CFGNode*>;
	const SetType& redundantSet;

	const Value* lookup(const Value* v)
	{
		assert(v != nullptr);
		auto itr = eqMap.find(v);
		if (itr == eqMap.end())
			return nullptr;
		else
			return itr->second;
	}
public:
	CFGAdjuster(const MapType& m, const SetType& s): eqMap(m), redundantSet(s) {}

	void visitEntryNode(EntryCFGNode&) {}
	void visitAllocNode(AllocCFGNode&) {}
	void visitCopyNode(CopyCFGNode& copyNode)
	{
		if (redundantSet.count(&copyNode))
			return;
		assert(!eqMap.count(copyNode.getDest()));

		std::vector<const Value*> newSrcs;
		newSrcs.reserve(copyNode.getNumSrc());
		SmallPtrSet<const Value*, 16> visitedSrcs;
		for (auto src: copyNode)
		{
			if (!visitedSrcs.insert(src).second)
				continue;
			if (auto newSrc = lookup(src))
				newSrcs.push_back(newSrc);
			else
				newSrcs.push_back(src);
		}

		copyNode.setSrc(std::move(newSrcs));
	}
	
	void visitOffsetNode(OffsetCFGNode& offsetNode)
	{
		if (redundantSet.count(&offsetNode))
			return;
		assert(!eqMap.count(offsetNode.getDest()));

		if (auto newSrc = lookup(offsetNode.getSrc()))
			offsetNode.setSrc(newSrc);
	}

	void visitLoadNode(LoadCFGNode& loadNode)
	{
		assert(!eqMap.count(loadNode.getDest()));
		if (auto newSrc = lookup(loadNode.getSrc()))
			loadNode.setSrc(newSrc);
	}

	void visitStoreNode(StoreCFGNode& storeNode)
	{
		if (auto newDest = lookup(storeNode.getDest()))
			storeNode.setDest(newDest);
		if (auto newSrc = lookup(storeNode.getSrc()))
			storeNode.setSrc(newSrc);
	}

	void visitCallNode(CallCFGNode& callNode)
	{
		assert(!eqMap.count(callNode.getDest()));
		if (auto newFunc = lookup(callNode.getFunctionPointer()))
			callNode.setFunctionPointer(newFunc);
		for (auto i = 0u, e = callNode.getNumArgument(); i < e; ++i)
		{
			if (auto newArg = lookup(callNode.getArgument(i)))
				callNode.setArgument(i, newArg);
		}
	}

	void visitReturnNode(ReturnCFGNode& retNode)
	{
		if (auto retVal = retNode.getReturnValue())
		{
			if (auto newVal = lookup(retVal))
				retNode.setReturnValue(newVal);
		}
	}
};

}

std::vector<CFGNode*> CFGSimplifier::findRedundantNodes(CFG& cfg)
{
	std::vector<CFGNode*> ret;
	for (auto node: cfg)
	{
		if (node->isCopyNode())
		{
			auto copyNode = static_cast<CopyCFGNode*>(node);
			if (copyNode->getNumSrc() == 1u)
			{
				ret.push_back(copyNode);
				eqMap[copyNode->getDest()] = copyNode->getSrc(0);
			}
		}

		if (node->isOffsetNode())
		{
			auto offsetNode = static_cast<OffsetCFGNode*>(node);
			if (offsetNode->getOffset() == 0u)
			{
				ret.push_back(offsetNode);
				eqMap[offsetNode->getDest()] = offsetNode->getSrc();
			}
		}
	}
	return ret;
}

void CFGSimplifier::flattenEquivalentMap()
{
	auto find = [this] (const Value* val)
	{
		while (true)
		{
			auto itr = eqMap.find(val);
			if (itr == eqMap.end())
				break;
			else
				val = itr->second;
		}
		assert(val != nullptr);
		return val;
	};

	for (auto& mapping: eqMap)
		mapping.second = find(mapping.second);
}

void CFGSimplifier::adjustCFG(CFG& cfg, const util::VectorSet<CFGNode*>& redundantNodes)
{
	auto adjuster = CFGAdjuster(eqMap, redundantNodes);
	for (auto node: cfg)
		adjuster.visit(*node);
}

void CFGSimplifier::adjustDefUseChain(const util::VectorSet<tpa::CFGNode*>& redundantNodes)
{
	for (auto node: redundantNodes)
	{
		// node may be assigned #null or #universal, in which cases the def size may be 0
		if (node->def_size() > 0u)
		{
			assert(node->def_size() == 1u);
			auto defNode = *node->def_begin();
			defNode->removeDefUseEdge(node);
			for (auto useNode: node->uses())
				defNode->insertDefUseEdge(useNode);
		}

		auto uses = SmallVector<CFGNode*, 8>(node->use_begin(), node->use_end());
		for (auto useNode: uses)
			node->removeDefUseEdge(useNode);

		assert(node->def_size() == 0u && node->use_size() == 0u);
	}
}

void CFGSimplifier::removeNodes(CFG& cfg, const util::VectorSet<CFGNode*>& redundantNodes)
{
	cfg.removeNodes(redundantNodes);
}

void CFGSimplifier::simplify(CFG& cfg)
{
	while (true)
	{
		auto redundantNodes = util::VectorSet<CFGNode*>(findRedundantNodes(cfg));
		if (redundantNodes.empty())
			break;
		
		flattenEquivalentMap();
		adjustCFG(cfg, redundantNodes);
		adjustDefUseChain(redundantNodes);
		removeNodes(cfg, redundantNodes);

		eqMap.clear();
	}
}

}