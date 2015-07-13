#include "Transforms/FoldIntToPtr.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PatternMatch.h>

using namespace llvm;
using namespace llvm::PatternMatch;

namespace transform
{

static bool foldInstruction(IntToPtrInst* inst)
{
	auto op = inst->getOperand(0)->stripPointerCasts();

	// Pointer copy: Y = inttoptr (ptrtoint X)
	Value* src = nullptr;
	if (match(op, m_PtrToInt(m_Value(src))))
	{
		inst->replaceAllUsesWith(src);
		inst->eraseFromParent();
		return true;
	}

	// Pointer arithmetic
	Value* offsetValue = nullptr;
	if (match(op, m_Add(m_PtrToInt(m_Value(src)), m_Value(offsetValue))))
	{
		if (src->getType() != inst->getType())
			src = new BitCastInst(src, inst->getType(), "src.cast", inst);
		auto gepInst = GetElementPtrInst::Create(src, { ConstantInt::get(Type::getInt64Ty(inst->getContext()), 0), offsetValue}, inst->getName(), inst);
		inst->replaceAllUsesWith(gepInst);
		inst->eraseFromParent();
		return true;
	}

	return false;
}

bool FoldIntToPtrPass::runOnBasicBlock(BasicBlock& bb)
{
	bool modified = false;
	for (auto itr = bb.begin(); itr != bb.end(); )
	{
		auto inst = itr++;
		if (auto itpInst = dyn_cast<IntToPtrInst>(inst))
			modified |= foldInstruction(itpInst);
	}
	return modified;
}

char FoldIntToPtrPass::ID = 0;
static RegisterPass<FoldIntToPtrPass> X("fold-inttoptr", "Turn ptrtoint+arithmetic+inttoptr into gep", false, false);

}
