#pragma once

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class BasicBlock;
	class Function;
	class Instruction;
	class Value;
}

namespace tpa
{

class CFG;
class CFGNode;
class InstructionTranslator;

class FunctionTranslator
{
private:
	CFG& cfg;
	InstructionTranslator& translator;

	// Map llvm basic block to the start/end CFGNode that corresponds to the block
	llvm::DenseMap<const llvm::BasicBlock*, std::pair<CFGNode*, CFGNode*>> bbToNode;
	llvm::DenseMap<const llvm::BasicBlock*, std::vector<CFGNode*>> nonEmptySuccMap;

	// Map CFGNode to correponding llvm instruction
	llvm::DenseMap<const CFGNode*, const llvm::Instruction*> nodeToInst;
	// Map llvm instruction to correponding CFGNode
	llvm::DenseMap<const llvm::Instruction*, CFGNode*> instToNode;

	void drawDefUseEdgeFromValue(const llvm::Value*, CFGNode*);

	void translateBasicBlock(const llvm::Function&);
	void processEmptyBlock();
	void connectCFGNodes(const llvm::BasicBlock&);
	void constructDefUseChains();
	void computeNodePriority();
	void detachStorePreservingNodes();
public:
	FunctionTranslator(CFG& c, InstructionTranslator& t): cfg(c), translator(t) {}

	void translateFunction(const llvm::Function&);
};

}
