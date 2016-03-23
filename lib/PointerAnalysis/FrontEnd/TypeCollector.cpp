#include "PointerAnalysis/FrontEnd/Type/TypeCollector.h"

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

class TypeSetBuilder
{
private:
	const Module& module;
	TypeSet& typeSet;

	DenseSet<const Value*> visitedValues;

	void incorporateFunctionType(FunctionType*);
	void incorporateStructType(StructType*);
	void incorporateArrayType(ArrayType*);
	void incorporatePointerType(PointerType*);
	void incorporateType(Type*);

	void incorporateConstant(const Constant*);
	void incorporateInstruction(const Instruction*);
	void incorporateValue(const Value*);
public:
	TypeSetBuilder(const Module& m, TypeSet& t): module(m), typeSet(t) {}

	void collectType();
};

void TypeSetBuilder::incorporateConstant(const Constant* constant)
{
	// Skip global value
	if (isa<GlobalValue>(constant))
		return;

	// Already visited?
	if (!visitedValues.insert(constant).second)
		return;

	// Check the type
	incorporateType(constant->getType());

	// Look in operands for types.
	for (auto const& op: constant->operands())
		incorporateValue(op);
}

void TypeSetBuilder::incorporateInstruction(const Instruction* inst)
{
	// Already visited?
	if (!visitedValues.insert(inst).second)
		return;

	// Check the type
	incorporateType(inst->getType());

	if (auto allocInst = dyn_cast<AllocaInst>(inst))
		incorporateType(allocInst->getAllocatedType());

	// Look in operands for types.
	for (auto const& op: inst->operands())
	{
		if (!isa<Instruction>(op))
			incorporateValue(op);
	}
}

void TypeSetBuilder::incorporateValue(const Value* value)
{
	if (auto constant = dyn_cast<Constant>(value))
		incorporateConstant(constant);
	else if (auto inst = dyn_cast<Instruction>(value))
		incorporateInstruction(inst);
}

void TypeSetBuilder::incorporateFunctionType(FunctionType* funType)
{
	for (auto pType: funType->params())
		incorporateType(pType);
}

void TypeSetBuilder::incorporateStructType(StructType* stType)
{
	for (auto elemType: stType->elements())
		incorporateType(elemType);
}

void TypeSetBuilder::incorporateArrayType(ArrayType* arrayType)
{
	incorporateType(arrayType->getElementType());
}

void TypeSetBuilder::incorporatePointerType(PointerType* ptrType)
{
	incorporateType(ptrType->getElementType());
}

void TypeSetBuilder::incorporateType(Type* llvmType)
{
	// We don't care about void type
	if (llvmType->isVoidTy())
		return;

	// Check to see if we've already visited this type.
	if (!typeSet.insert(llvmType))
		return;

	if (auto ptrType = dyn_cast<PointerType>(llvmType))
		incorporatePointerType(ptrType);
	else if (auto funType = dyn_cast<FunctionType>(llvmType))
		incorporateFunctionType(funType);
	else if (auto stType = dyn_cast<StructType>(llvmType))
		incorporateStructType(stType);
	else if (auto arrType = dyn_cast<ArrayType>(llvmType))
		incorporateArrayType(arrType);
	else if (llvmType->isVectorTy())
		llvm_unreachable("Vector type not supported");
}

void TypeSetBuilder::collectType()
{
	// Get types from global variables
	for (auto const& global: module.globals())
	{
		incorporateType(global.getType());
		if (global.hasInitializer())
			incorporateValue(global.getInitializer());
	}

	// Get types from functions
	for (auto const& f: module)
	{
		assert(!f.hasPrefixData() && !f.hasPrologueData());

		incorporateType(f.getType());

		for (auto const& bb: f)
			for (auto const& inst: bb)
				incorporateValue(&inst);
	}
}

}

TypeSet TypeCollector::runOnModule(const Module& module)
{
	TypeSet typeSet(module);

	TypeSetBuilder(module, typeSet).collectType();

	return typeSet;
}

}
