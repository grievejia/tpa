//===- ExpandConstantExpr.cpp - Convert ConstantExprs to Instructions------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass expands out ConstantExprs into Instructions.
//
// Note that this only converts ConstantExprs that are referenced by
// Instructions.  It does not convert ConstantExprs that are used as
// initializers for global variables.
//
// This simplifies the language so that the later analyses do not
// need to handle ConstantExprs when scanning through instructions
//
//===----------------------------------------------------------------------===//

#include "ExpandConstantExpr.h"
#include "ExpandUtils.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

using namespace llvm;

static bool expandInstruction(Instruction* inst);

static Value *expandConstantExpr(Instruction* insertPt, ConstantExpr* expr)
{
	Instruction* newInst = expr->getAsInstruction();
	newInst->insertBefore(insertPt);
	newInst->setName("expanded");
	expandInstruction(newInst);
	return newInst;
}

static bool expandInstruction(Instruction *inst)
{
	// A landingpad can only accept ConstantExprs, so it should remain
	// unmodified.
	if (isa<LandingPadInst>(inst))
		return false;

	bool modified = false;
	for (unsigned opNum = 0; opNum < inst->getNumOperands(); ++opNum)
	{
		if (ConstantExpr *expr = dyn_cast<ConstantExpr>(inst->getOperand(opNum)))
		{
			modified = true;
			Use* user = &inst->getOperandUse(opNum);
			phiSafeReplaceUses(user, expandConstantExpr(phiSafeInsertPt(user), expr));
		}
	}
	return modified;
}

bool ExpandConstantExprPass::runOnFunction(Function& F)
{
	bool modified = false;
	for (auto& bb: F)
	{
		for (auto& inst: bb)
			modified |= expandInstruction(&inst);
	}
	return modified;
}

char ExpandConstantExprPass::ID = 0;
static RegisterPass<ExpandConstantExprPass> X("expand-constant-expr", "Expand out ConstantExprs into Instructions", false, false);
