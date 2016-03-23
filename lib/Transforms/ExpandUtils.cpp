//===-- ExpandUtils.cpp - Helper functions for expansion passes -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExpandUtils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace transform
{

Instruction *phiSafeInsertPt(Use* use)
{
	Instruction* insertPt = cast<Instruction>(use->getUser());
	if (PHINode* phiNode = dyn_cast<PHINode>(insertPt))
	{
		// We cannot insert instructions before a PHI node, so insert
		// before the incoming block's terminator.  This could be
		// suboptimal if the terminator is a conditional.
		insertPt = phiNode->getIncomingBlock(*use)->getTerminator();
	}
	return insertPt;
}

void phiSafeReplaceUses(Use* use, Value* newVal)
{
	if (PHINode* phiNode = dyn_cast<PHINode>(use->getUser()))
	{
		// A PHI node can have multiple incoming edges from the same
		// block, in which case all these edges must have the same
		// incoming value.
		BasicBlock* bb = phiNode->getIncomingBlock(*use);
		for (unsigned i = 0; i < phiNode->getNumIncomingValues(); ++i)
		{
			if (phiNode->getIncomingBlock(i) == bb)
				phiNode->setIncomingValue(i, newVal);
		}
	}
	else
	{
		use->getUser()->replaceUsesOfWith(use->get(), newVal);
	}
}

Function* recreateFunction(Function* func, FunctionType* newType)
{
	Function* newFunc = Function::Create(newType, func->getLinkage());
	newFunc->copyAttributesFrom(func);
	func->getParent()->getFunctionList().insert(func, newFunc);
	newFunc->takeName(func);
	newFunc->getBasicBlockList().splice(newFunc->begin(),
										func->getBasicBlockList());
	func->replaceAllUsesWith(
		ConstantExpr::getBitCast(newFunc,
								 func->getFunctionType()->getPointerTo()));
	return newFunc;
}

Instruction* copyDebug(Instruction* newInst, Instruction* original)
{
	newInst->setDebugLoc(original->getDebugLoc());
	return newInst;
}

}
