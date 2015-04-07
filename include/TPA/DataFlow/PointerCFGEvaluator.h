#ifndef TPA_POINTER_CFG_EVALUATOR_H
#define TPA_POINTER_CFG_EVALUATOR_H

#include "PointerAnalysis/ControlFlow/NodeVisitor.h"
#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TPAWorkList.h"

namespace tpa
{

class Context;
class PointerCFGNode;

class PointerCFGEvaluator: public NodeVisitor<PointerCFGEvaluator>
{
private:
	const Context* ctx;
	const PointerCFG* cfg;

	SemiSparseGlobalState& globalState;

	using GlobalWorkListType = TPAWorkList<PointerCFG>;
	using LocalWorkListType = typename GlobalWorkListType::LocalWorkList;
	GlobalWorkListType& globalWorkList;
	LocalWorkListType& localWorkList;

	// NodeVisitor implementations
	void visitEntryNode(const EntryNode*);
	void visitAllocNode(const AllocNode*);
	void visitCopyNode(const CopyNode*);
	void visitLoadNode(const LoadNode*);
	void visitStoreNode(const StoreNode*);
	void visitCallNode(const CallNode*);
	void visitReturnNode(const ReturnNode*);

	// Inter-proc processing
	void applyCall(const CallNode* callNode, const llvm::Function* callee, const Store& store);
	void applyExternalCall(const CallNode* callNode, const Context* newCtx, const llvm::Function* callee, const Store& store);
	void applyNonExternalCall(const CallNode* callNode, const Context* newCtx, const llvm::Function* callee, const Store& store);
	void applyReturn(const ReturnNode* retNode, const Context* oldCtx, const llvm::Instruction* oldInst, const Store& store);

	// State propagations
	void propagateTopLevel(const PointerCFGNode* node);
	void propagateMemLevel(const PointerCFGNode* node, const Store& store);
	void propagateTopLevelGlobal(const Context* oldCtx, const PointerCFG* oldCFG, const PointerCFG* node);
	void propagateMemLevelGlobal(const Context* oldCtx, const PointerCFG* oldCFG, const PointerCFG* node, const Store& store);

	// Make sure NodeVisitor can access the private members of this class
	friend class NodeVisitor<PointerCFGEvaluator>;
public:
	PointerCFGEvaluator(const Context* c, const PointerCFG* p, SemiSparseGlobalState& gs, GlobalWorkListType& wl);

	void eval();
};

}

#endif
