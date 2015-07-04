#include "PointerAnalysis/FrontEnd/TypeAnalysis.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"

#include <llvm/ADT/DenseSet.h>
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

class TypeMapBuilder
{
private:
	const Module& module;
	TypeMap& typeMap;

	DataLayout dataLayout;
	DenseSet<const Value*> visitedValues;

	const TypeLayout* insertTypeMap(Type*, const TypeLayout*);
	const TypeLayout* constructPointerLayout(Type*, size_t, const ArrayLayout*, const PointerLayout*);
	const TypeLayout* incorporateSimpleType(Type*);
	const TypeLayout* incorporatePointerType(PointerType*);
	const TypeLayout* incorporateStructType(StructType*);
	const TypeLayout* incorporateArrayType(ArrayType*);
	const TypeLayout* incorporateType(Type*);

	void incorporateConstant(const Constant*);
	void incorporateInstruction(const Instruction*);
	void incorporateValue(const Value*);
public:
	TypeMapBuilder(const Module& m, TypeMap& t): module(m), typeMap(t), dataLayout(&m) {}

	void buildTypeMap();
};

const TypeLayout* TypeMapBuilder::insertTypeMap(Type* key, const TypeLayout* val)
{
	assert(key != nullptr && val != nullptr);
	typeMap.insert(key, val);
	return val;
}

const TypeLayout* TypeMapBuilder::constructPointerLayout(Type* ty, size_t sz, const ArrayLayout* al, const PointerLayout* pl)
{
	auto mappedType = TypeLayout::getTypeLayout(sz, al, pl);
	return insertTypeMap(ty, mappedType);
}

void TypeMapBuilder::incorporateConstant(const Constant* constant)
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

void TypeMapBuilder::incorporateInstruction(const Instruction* inst)
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

void TypeMapBuilder::incorporateValue(const Value* value)
{
	if (auto constant = dyn_cast<Constant>(value))
		incorporateConstant(constant);
	else if (auto inst = dyn_cast<Instruction>(value))
		incorporateInstruction(inst);
}

const TypeLayout* TypeMapBuilder::incorporateSimpleType(Type* type)
{
	auto typeSize = dataLayout.getTypeAllocSize(type);
	return constructPointerLayout(type, typeSize, tpa::ArrayLayout::getDefaultLayout(), tpa::PointerLayout::getEmptyLayout());
}

const TypeLayout* TypeMapBuilder::incorporatePointerType(PointerType* ptrType)
{
	auto typeSize = dataLayout.getTypeAllocSize(ptrType);
	return constructPointerLayout(ptrType, typeSize, tpa::ArrayLayout::getDefaultLayout(), tpa::PointerLayout::getSinglePointerLayout());
}

const TypeLayout* TypeMapBuilder::incorporateStructType(StructType* stType)
{
	// Put a placeholder in typeMap to avoid infinite loop
	typeMap.insert(stType, nullptr);

	// We know nothing about opaque type. Conservatively treat it as a byte array
	if (stType->isOpaque())
		return insertTypeMap(stType, TypeLayout::getByteArrayTypeLayout());

	tpa::ArrayLayout::ArrayTripleList arrayTripleList;
	util::VectorSet<size_t> ptrOffsets;

	auto structLayout = dataLayout.getStructLayout(stType);
	for (unsigned i = 0, e = stType->getNumElements(); i != e; ++i)
	{
		auto subType = stType->getElementType(i);
		auto mappedSubType = incorporateType(subType);
		assert(mappedSubType != nullptr);
		auto offset = structLayout->getElementOffset(i);
		
		// Update array layout
		auto subLayout = mappedSubType->getArrayLayout();
		if (!subLayout->empty())
		{
			// Adjust the start and end field in the sub-layout according to the offset
			std::transform(
				subLayout->begin(),
				subLayout->end(),
				std::back_inserter(arrayTripleList),
				[offset] (auto const& triple) -> tpa::ArrayTriple
				{
					return { triple.start + offset, triple.end + offset, triple.size };
				}
			);
		}

		// Update pointer layout
		auto ptrLayout = mappedSubType->getPointerLayout();
		for (auto subOffset: *ptrLayout)
			ptrOffsets.insert(subOffset + offset);
	}

	auto stSize = dataLayout.getTypeAllocSize(stType);
	auto stArrayLayout = ArrayLayout::getLayout(std::move(arrayTripleList));
	auto stPtrLayout = PointerLayout::getLayout(std::move(ptrOffsets));
	return constructPointerLayout(stType, stSize, stArrayLayout, stPtrLayout);

}

const TypeLayout* TypeMapBuilder::incorporateArrayType(ArrayType* arrayType)
{
	auto elemType = arrayType->getElementType();
	auto numElems = arrayType->getNumElements();
	auto elemSize = dataLayout.getTypeAllocSize(elemType);

	auto arraySize = numElems * elemSize;
	tpa::ArrayLayout::ArrayTripleList arrayTripleList = {{0, arraySize, elemSize}};
	// We need to examine the element type here because it may contain additional array triple
	auto mappedSubType = incorporateType(elemType);
	assert(mappedSubType != nullptr);
	auto subLayout = mappedSubType->getArrayLayout();
	if (!subLayout->empty())
		arrayTripleList.insert(arrayTripleList.end(), subLayout->begin(), subLayout->end());

	// Remove duplicate entries in triple list
	// TODO: arrays with size 1 are not "real" arrays. Remove them
	arrayTripleList.erase(std::unique(arrayTripleList.begin(), arrayTripleList.end()), arrayTripleList.end());

	auto arrayLayout = tpa::ArrayLayout::getLayout(std::move(arrayTripleList));
	return constructPointerLayout(arrayType, arraySize, arrayLayout, mappedSubType->getPointerLayout());
}

const TypeLayout* TypeMapBuilder::incorporateType(Type* llvmType)
{
	// Check to see if we've already visited this type.
	auto layout = typeMap.lookup(llvmType);
	if (layout != nullptr)
		return layout;

	switch (llvmType->getTypeID())
	{
		case Type::VoidTyID:
			return nullptr;
		case Type::HalfTyID:
		case Type::FloatTyID:
		case Type::DoubleTyID:
		case Type::X86_FP80TyID:
		case Type::FP128TyID:
		case Type::PPC_FP128TyID:
		case Type::LabelTyID:
		case Type::MetadataTyID:
		case Type::X86_MMXTyID:
		case Type::IntegerTyID:
			return incorporateSimpleType(llvmType);
		case Type::PointerTyID:
			return incorporatePointerType(cast<PointerType>(llvmType));
		case Type::FunctionTyID:
		{
			auto funType = cast<FunctionType>(llvmType);
			for (auto pType: funType->params())
				incorporateType(pType);
			return constructPointerLayout(funType, dataLayout.getPointerSize(), tpa::ArrayLayout::getDefaultLayout(), tpa::PointerLayout::getSinglePointerLayout());
		}
		case Type::StructTyID:
			return incorporateStructType(cast<StructType>(llvmType));
		case Type::ArrayTyID:
			return incorporateArrayType(cast<ArrayType>(llvmType));
		case Type::VectorTyID:
			llvm_unreachable("Vector type not supported");
	}
}

void TypeMapBuilder::buildTypeMap()
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

TypeMap TypeAnalysis::runOnModule(const Module& module)
{
	TypeMap typeMap;

	TypeMapBuilder(module, typeMap).buildTypeMap();

	return typeMap;
}

}
