#ifndef TPA_EXPAND_UTILS_H
#define TPA_EXPAND_UTILS_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

llvm::Instruction* phiSafeInsertPt(llvm::Use* use);
void phiSafeReplaceUses(llvm::Use* use, llvm::Value* newVal);
llvm::Function* recreateFunction(llvm::Function* func, llvm::FunctionType* newType);
llvm::Instruction* copyDebug(llvm::Instruction* newInst, llvm::Instruction* original);

template <class InstType>
static void copyLoadOrStoreAttrs(InstType* dest, InstType* src)
{
	dest->setVolatile(src->isVolatile());
	dest->setAlignment(src->getAlignment());
	dest->setOrdering(src->getOrdering());
	dest->setSynchScope(src->getSynchScope());
}

#endif
