//===- ExpandByVal.cpp - Expand out use of "byval" and "sret" attributes---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass expands out by-value passing of structs as arguments and
// return values.  In LLVM IR terms, it expands out the "byval" and
// "sret" function argument attributes.
//
// The semantics of the "byval" attribute are that the callee function
// gets a private copy of the pointed-to argument that it is allowed
// to modify.  In implementing this, we have a choice between making
// the caller responsible for making the copy or making the callee
// responsible for making the copy.  We choose the former, because
// this matches how the normal native calling conventions work, and
// because it often allows the caller to write struct contents
// directly into the stack slot that it passes the callee, without an
// additional copy.
//
// Note that this pass does not attempt to modify functions that pass
// structs by value without using "byval" or "sret", such as:
//
//   define %struct.X @func()                           ; struct return
//   define void @func(%struct.X %arg)                  ; struct arg
//
// The pass only handles functions such as:
//
//   define void @func(%struct.X* sret %result_buffer)  ; struct return
//   define void @func(%struct.X* byval %ptr_to_arg)    ; struct arg
//
// This is because clang generates the latter and not the former.
//
//===----------------------------------------------------------------------===//

#include "ExpandByVal.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

// removeAttribute() currently does not work on Attribute::Alignment
// (it fails with an assertion error), so we have to take a more
// convoluted route to removing this attribute by recreating the
// AttributeSet.
AttributeSet removeAttrs(LLVMContext& context, AttributeSet attrs)
{
	SmallVector<AttributeSet, 8> attrList;
	for (unsigned slot = 0; slot < attrs.getNumSlots(); ++slot)
	{
		unsigned index = attrs.getSlotIndex(slot);
		AttrBuilder ab;
		for (AttributeSet::iterator attr = attrs.begin(slot), atte = attrs.end(slot);
				 attr != atte; ++attr)
		{
			if (!attr->isIntAttribute() &&
					attr->isEnumAttribute() &&
					attr->getKindAsEnum() != Attribute::ByVal &&
					attr->getKindAsEnum() != Attribute::StructRet) {
				ab.addAttribute(*attr);
			}
			// IR semantics require that ByVal implies NoAlias.  However, IR
			// semantics do not require StructRet to imply NoAlias.  For
			// example, a global variable address can be passed as a
			// StructRet argument, although Clang does not do so and Clang
			// explicitly adds NoAlias to StructRet arguments.
			if (attr->isEnumAttribute() &&
					attr->getKindAsEnum() == Attribute::ByVal) {
				ab.addAttribute(Attribute::get(context, Attribute::NoAlias));
			}
		}
		attrList.push_back(AttributeSet::get(context, index, ab));
	}
	return AttributeSet::get(context, attrList);
}

// expandCall() can take a CallInst or an InvokeInst.  It returns
// whether the instruction was modified.
template <class InstType>
static bool expandCall(DataLayout* dataLayout, InstType* call)
{
	bool modify = false;
	AttributeSet attrs = call->getAttributes();
	for (unsigned argIdx = 0; argIdx < call->getNumArgOperands(); ++argIdx) {
		unsigned attrIdx = argIdx + 1;

		if (attrs.hasAttribute(attrIdx, Attribute::StructRet))
			modify = true;

		if (attrs.hasAttribute(attrIdx, Attribute::ByVal))
		{
			modify = true;

			Value* argPtr = call->getArgOperand(argIdx);
			Type* argType = argPtr->getType()->getPointerElementType();
			ConstantInt* argSize = ConstantInt::get(call->getContext(), APInt(64, dataLayout->getTypeStoreSize(argType)));
			unsigned alignment = attrs.getParamAlignment(attrIdx);
			// In principle, using the alignment from the argument attribute
			// should be enough.  However, Clang is not emitting this
			// attribute for PNaCl.  LLVM alloca instructions do not use the
			// ABI alignment of the type, so this must be specified
			// explicitly.
			// See https://code.google.com/p/nativeclient/issues/detail?id=3403
			unsigned allocAlignment = std::max(alignment, dataLayout->getABITypeAlignment(argType));

			// Make a copy of the byval argument.
			Instruction* copyBuf = new AllocaInst(argType, 0, allocAlignment, argPtr->getName() + ".byval_copy");
			Function* func = call->getParent()->getParent();
			func->getEntryBlock().getInstList().push_front(copyBuf);
			IRBuilder<> builder(call);
			// Using the argument's alignment attribute for the memcpy
			// should be OK because the LLVM Language Reference says that
			// the alignment attribute specifies "the alignment of the stack
			// slot to form and the known alignment of the pointer specified
			// to the call site".
			Instruction* memCpy = builder.CreateMemCpy(copyBuf, argPtr, argSize, alignment);
			memCpy->setDebugLoc(call->getDebugLoc());

			call->setArgOperand(argIdx, copyBuf);
		}
	}
	if (modify)
	{
		call->setAttributes(removeAttrs(call->getContext(), attrs));

		if (CallInst* ci = dyn_cast<CallInst>(call))
		{
			// This is no longer a tail call because the callee references
			// memory alloca'd by the caller.
			ci->setTailCall(false);
		}
	}
	return modify;
}

bool ExpandByValPass::runOnModule(Module& M)
{
	bool modified = false;
	DataLayout dataLayout(&M);

	for (auto& func: M)
	{
		AttributeSet newAttrs = removeAttrs(func.getContext(), func.getAttributes());
		modified |= (newAttrs != func.getAttributes());
		func.setAttributes(newAttrs);

		for (auto& bb: func)
		{
			for (auto& inst: bb)
			{
				if (CallInst* call = dyn_cast<CallInst>(&inst))
				{
					modified |= expandCall(&dataLayout, call);
				}
				else if (InvokeInst* call = dyn_cast<InvokeInst>(&inst))
				{
					modified |= expandCall(&dataLayout, call);
				}
			}
		}
	}

	return modified;
}

char ExpandByValPass::ID = 0;
static RegisterPass<ExpandByValPass> X("expand-byval", "Expand out by-value passing of structs", false, false);
