//===- ExpandGetElementPtr.cpp - Expand GetElementPtr------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass break down complex GetElementPtr instructions into smaller ones
//
// By "complex" I mean those GEPs that perform both constant-offset pointer arithmetic and variable-offset (reference an array with a variable index) pointer arithmetic.
//
// After this pass, all GEPs in the IR are guaranteed to be in one of the following forms:
//
// - A constant-offset GEP, whose offset can be retrieved easily using GetPointerBaseWithConstantOffset(). (e.g. y = getelementptr x, 1, 2, 3, 4)
// - A variable-offset GEP with one index argument, which is a variable. (e.g. y = getelementptr x, i)
// - A variable-offset GEP with two index argument, where the first index argument is 0 and the second one is a variable. (e.g. y = getelementptr x, 0, i)
//
//===----------------------------------------------------------------------===//

#include "ExpandGetElementPtr.h"
#include "ExpandUtils.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Analysis/ValueTracking.h"

using namespace llvm;

static bool expandGEP(GetElementPtrInst* gepInst, const DataLayout* dataLayout, Type* ptrType)
{
	int64_t offset = 0;
	bool isInbound = gepInst->isInBounds();
	Value* basePtr = GetPointerBaseWithConstantOffset(gepInst, offset, dataLayout);

	// If we can be sure that gepInst is a constant-offset GEP, just ignore it
	if (basePtr != gepInst)
		return false;

	if (gepInst->getNumOperands() <= 3)
		if (auto cInt = dyn_cast<ConstantInt>(gepInst->getOperand(1)))
			if (cInt->isZero())
				return false;

	//errs() << "OPERATION: " << *gepInst << "\n";

	// For GEPs that has variable as an offset, simply take it out and leave all other offsets alone
	std::vector<unsigned> partitions;
	for (unsigned i = 1u, e = gepInst->getNumOperands(); i < e; ++i)
	{
		auto index = gepInst->getOperand(i);

		if (!isa<ConstantInt>(index))
			partitions.push_back(i);
	}

	assert(!partitions.empty());
	if (partitions.size() == 1 && gepInst->getNumOperands() == 2)
		return false;

	auto ptr = gepInst->getPointerOperand();
	unsigned start = 0;
	unsigned lastSplit = 1;
	if (partitions[0] == 1)
	{
		std::vector<Value*> indices = { gepInst->getOperand(1) };
		ptr = GetElementPtrInst::Create(ptr, indices, "gep_array_var", gepInst);
		cast<GetElementPtrInst>(ptr)->setIsInBounds(isInbound);
		//errs() << "\tvar = " << *ptr << "\n";
		start = 1;
		lastSplit = 2;
	}
	
	for (unsigned i = start, e = partitions.size(); i < e; ++i)
	{
		auto splitPoint = partitions[i];

		std::vector<Value*> indices;
		if (lastSplit != 1u)
			indices.push_back(ConstantInt::get(ptrType, 0));

		for (auto i = lastSplit; i < splitPoint; ++i)
			indices.push_back(gepInst->getOperand(i));

		if (!indices.empty() && !(indices.size() == 1 && isa<ConstantInt>(indices[0]) && cast<ConstantInt>(indices[0])->isZero()))
		{
			ptr = GetElementPtrInst::Create(ptr, indices, "gep_array_const", gepInst);
			cast<GetElementPtrInst>(ptr)->setIsInBounds(isInbound);
			//errs() << "\tconst = " << *ptr << "\n";
		}
		indices.clear();

		indices.push_back(ConstantInt::get(ptrType, 0));
		indices.push_back(gepInst->getOperand(splitPoint));
		ptr = GetElementPtrInst::Create(ptr, indices, "gep_array_var", gepInst);
		cast<GetElementPtrInst>(ptr)->setIsInBounds(isInbound);
		//errs() << "\tvar = " << *ptr << "\n";

		lastSplit = splitPoint + 1;
	}

	if (lastSplit < gepInst->getNumOperands())
	{
		std::vector<Value*> indices = { ConstantInt::get(ptrType, 0) };
		for (auto i = lastSplit; i < gepInst->getNumOperands(); ++i)
			indices.push_back(gepInst->getOperand(i));
		ptr = GetElementPtrInst::Create(ptr, indices, "gep_array_const", gepInst);
		cast<GetElementPtrInst>(ptr)->setIsInBounds(isInbound);
		//errs() << "\tconst = " << *ptr << "\n";
	}

	ptr->takeName(gepInst);
	gepInst->replaceAllUsesWith(ptr);
	gepInst->eraseFromParent();

	return true;
}

bool ExpandGetElementPtrPass::runOnBasicBlock(BasicBlock& BB)
{
	bool modified = false;
	DataLayout dataLayout(BB.getParent()->getParent());
	Type* ptrType = dataLayout.getIntPtrType(BB.getContext());

	for (BasicBlock::InstListType::iterator itr = BB.begin(); itr != BB.end(); )
	{
		Instruction* inst = itr++;
		if (GetElementPtrInst* gepInst = dyn_cast<GetElementPtrInst>(inst))
		{
			modified |= expandGEP(gepInst, &dataLayout, ptrType);
		}
	}
	return modified;
}

char ExpandGetElementPtrPass::ID = 0;
static RegisterPass<ExpandGetElementPtrPass> X("expand-gep", "Expand out GetElementPtr instructions into arithmetic", false, false);
