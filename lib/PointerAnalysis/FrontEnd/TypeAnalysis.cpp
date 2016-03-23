#include "PointerAnalysis/FrontEnd/Type/ArrayLayoutAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/PointerLayoutAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/StructCastAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/TypeAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/TypeCollector.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"

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

	size_t getTypeSize(Type*, const DataLayout&);
	void insertTypeMap(Type*, size_t, const ArrayLayout*, const PointerLayout*);
	void insertOpaqueType(Type*);
public:
	TypeMapBuilder(const Module& m, TypeMap& t): module(m), typeMap(t) {}

	void buildTypeMap();
};

void TypeMapBuilder::insertOpaqueType(Type* type)
{
	typeMap.insert(type, TypeLayout::getByteArrayTypeLayout());
}

void TypeMapBuilder::insertTypeMap(Type* type, size_t size, const ArrayLayout* arrayLayout, const PointerLayout* ptrLayout)
{
	auto typeLayout = TypeLayout::getTypeLayout(size, arrayLayout, ptrLayout);
	typeMap.insert(type, typeLayout);
}

size_t TypeMapBuilder::getTypeSize(Type* type, const DataLayout& dataLayout)
{
	if (isa<FunctionType>(type))
		return dataLayout.getPointerSize();
	else
	{
		while (auto arrayType = dyn_cast<ArrayType>(type))
			type = arrayType->getElementType();
		return dataLayout.getTypeAllocSize(type);
	}
}

void TypeMapBuilder::buildTypeMap()
{
	auto typeSet = TypeCollector().runOnModule(module);
	auto structCastMap = StructCastAnalysis().runOnModule(module);
	auto arrayLayoutMap = ArrayLayoutAnalysis().runOnTypes(typeSet);
	auto ptrLayoutMap = PointerLayoutAnalysis(structCastMap).runOnTypes(typeSet);

	for (auto type: typeSet)
	{
		if (auto stType = dyn_cast<StructType>(type))
		{
			if (stType->isOpaque())
			{
				insertOpaqueType(type);
				continue;
			}
		}

		auto typeSize = getTypeSize(type, typeSet.getDataLayout());

		auto ptrLayout = ptrLayoutMap.lookup(type);
		assert(ptrLayout != nullptr);

		auto arrayLayout = arrayLayoutMap.lookup(type);
		assert(arrayLayout != nullptr);

		insertTypeMap(type, typeSize, arrayLayout, ptrLayout);
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
