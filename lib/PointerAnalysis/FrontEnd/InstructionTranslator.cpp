#include "Context/Context.h"
#include "PointerAnalysis/FrontEnd/CFG/InstructionTranslator.h"
#include "PointerAnalysis/FrontEnd/Type/TypeMap.h"
#include "PointerAnalysis/Program/CFG/CFG.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

tpa::CFGNode* InstructionTranslator::createCopyNode(const Instruction* inst, const SmallPtrSetImpl<const Value*>& srcs)
{
	assert(inst != nullptr && srcs.size() > 0u);
	auto srcVals = std::vector<const Value*>(srcs.begin(), srcs.end());
	return cfg.create<tpa::CopyCFGNode>(inst, std::move(srcVals));
}

tpa::CFGNode* InstructionTranslator::visitAllocaInst(AllocaInst& allocaInst)
{
	assert(allocaInst.getType()->isPointerTy());

	auto allocType = typeMap.lookup(allocaInst.getAllocatedType());
	assert(allocType != nullptr && "Alloc type not found");

	return cfg.create<tpa::AllocCFGNode>(&allocaInst, allocType);
}

tpa::CFGNode* InstructionTranslator::visitLoadInst(LoadInst& loadInst)
{
	if (!loadInst.getType()->isPointerTy())
		return nullptr;

	auto dstVal = &loadInst;
	auto srcVal = loadInst.getPointerOperand()->stripPointerCasts();
	return cfg.create<tpa::LoadCFGNode>(dstVal, srcVal);
}

tpa::CFGNode* InstructionTranslator::visitStoreInst(StoreInst& storeInst)
{
	auto valOp = storeInst.getValueOperand();
	if (!valOp->getType()->isPointerTy())
		return nullptr;
	auto ptrOp = storeInst.getPointerOperand();

	return cfg.create<tpa::StoreCFGNode>(ptrOp->stripPointerCasts(), valOp->stripPointerCasts());
}

tpa::CFGNode* InstructionTranslator::visitReturnInst(ReturnInst& retInst)
{
	auto retVal = retInst.getReturnValue();
	if (retVal != nullptr)
		retVal = retVal->stripPointerCasts();

	auto retNode = cfg.create<tpa::ReturnCFGNode>(retVal);
	cfg.setExitNode(retNode);
	return retNode;
}

tpa::CFGNode* InstructionTranslator::visitCallSite(CallSite cs)
{
	auto funPtr = cs.getCalledValue()->stripPointerCasts();

	// The reinterpret_cast here just use the instruction pointer to assign a unique id to the corresponding call site
	auto callNode = cfg.create<tpa::CallCFGNode>(funPtr, cs.getInstruction());

	for (unsigned i = 0; i < cs.arg_size(); ++i)
	{
		auto arg = cs.getArgument(i)->stripPointerCasts();

		if (!arg->getType()->isPointerTy())
			continue;

		callNode->addArgument(arg);
	}
	return callNode;
}

tpa::CFGNode* InstructionTranslator::visitPHINode(PHINode& phiInst)
{
	if (!phiInst.getType()->isPointerTy())
		return nullptr;

	auto srcs = SmallPtrSet<const Value*, 4>();
	for (unsigned i = 0; i < phiInst.getNumIncomingValues(); ++i)
	{
		auto value = phiInst.getIncomingValue(i)->stripPointerCasts();
		if (isa<UndefValue>(value))
			continue;
		srcs.insert(value);
	}

	return createCopyNode(&phiInst, srcs);
}

tpa::CFGNode* InstructionTranslator::visitSelectInst(SelectInst& selectInst)
{
	if (!selectInst.getType()->isPointerTy())
		return nullptr;

	auto srcs = SmallPtrSet<const Value*, 2>();
	srcs.insert(selectInst.getFalseValue()->stripPointerCasts());
	srcs.insert(selectInst.getTrueValue()->stripPointerCasts());
	
	return createCopyNode(&selectInst, srcs);
}

tpa::CFGNode* InstructionTranslator::visitGetElementPtrInst(GetElementPtrInst& gepInst)
{
	assert(gepInst.getType()->isPointerTy());

	auto srcVal = gepInst.getPointerOperand()->stripPointerCasts();

	// Constant-offset GEP
	auto gepOffset = APInt(dataLayout.getPointerTypeSizeInBits(srcVal->getType()), 0);
	if (gepInst.accumulateConstantOffset(dataLayout, gepOffset))
	{
		auto offset = gepOffset.getSExtValue();

		return cfg.create<tpa::OffsetCFGNode>(&gepInst, srcVal, offset, false);
	}

	// Variable-offset GEP
	auto numOps = gepInst.getNumOperands();
	if (numOps != 2 && numOps != 3)
		llvm_unreachable("Found a non-canonicalized GEP. Please run -expand-gep pass first!");

	size_t offset = dataLayout.getPointerSize();
	if (numOps == 2)
		offset = dataLayout.getTypeAllocSize(cast<SequentialType>(srcVal->getType())->getElementType());
	else
	{
		assert(isa<ConstantInt>(gepInst.getOperand(1)) && cast<ConstantInt>(gepInst.getOperand(1))->isZero());
		auto elemType = cast<SequentialType>(gepInst.getPointerOperand()->getType())->getElementType();
		offset = dataLayout.getTypeAllocSize(cast<SequentialType>(elemType)->getElementType());
	}

	return cfg.create<tpa::OffsetCFGNode>(&gepInst, srcVal, offset, true);
}

tpa::CFGNode* InstructionTranslator::visitIntToPtrInst(IntToPtrInst& inst)
{
	assert(inst.getType()->isPointerTy());

	// Common cases are handled in FoldIntToPtr prepass. In other cases, we make no effort to track what the rhs is. Assign UniversalPtr to the rhs
	std::vector<const llvm::Value*> srcs = { UndefValue::get(inst.getType()) };
	return cfg.create<tpa::CopyCFGNode>(&inst, std::move(srcs));
}

tpa::CFGNode* InstructionTranslator::visitBitCastInst(BitCastInst& bcInst)
{
	return nullptr;
}

tpa::CFGNode* InstructionTranslator::handleUnsupportedInst(const Instruction& inst)
{
	errs() << "inst = " << inst << "\n";
	llvm_unreachable("instruction not supported");
}

tpa::CFGNode* InstructionTranslator::visitExtractValueInst(ExtractValueInst& inst)
{
	if (!inst.getType()->isPointerTy())
		return nullptr;
	return handleUnsupportedInst(inst);
}
tpa::CFGNode* InstructionTranslator::visitInsertValueInst(InsertValueInst& inst)
{
	if (!inst.getType()->isPointerTy())
		return nullptr;
	return handleUnsupportedInst(inst);
}
tpa::CFGNode* InstructionTranslator::visitVAArgInst(VAArgInst& inst)
{
	return handleUnsupportedInst(inst);
}
tpa::CFGNode* InstructionTranslator::visitExtractElementInst(ExtractElementInst& inst)
{
	return handleUnsupportedInst(inst);
}
tpa::CFGNode* InstructionTranslator::visitInsertElementInst(InsertElementInst& inst)
{
	return handleUnsupportedInst(inst);
}
tpa::CFGNode* InstructionTranslator::visitShuffleVectorInst(ShuffleVectorInst& inst)
{
	return handleUnsupportedInst(inst);
}
tpa::CFGNode* InstructionTranslator::visitLandingPadInst(LandingPadInst& inst)
{
	return handleUnsupportedInst(inst);
}

}