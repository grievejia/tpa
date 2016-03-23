#include "PointerAnalysis/FrontEnd/Type/CastMap.h"
#include "PointerAnalysis/FrontEnd/Type/PointerLayoutAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/TypeSet.h"
#include "PointerAnalysis/MemoryModel/Type/PointerLayout.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

class PtrLayoutMapBuilder
{
private:
	const TypeSet& typeSet;
	PointerLayoutMap& ptrLayoutMap;

	void insertMap(const Type*, const PointerLayout*);

	const PointerLayout* processStructType(StructType*);
	const PointerLayout* processArrayType(ArrayType*);
	const PointerLayout* processPointerType(Type*);
	const PointerLayout* processNonPointerType(Type*);
	const PointerLayout* processType(Type*);
public:
	PtrLayoutMapBuilder(const TypeSet& t, PointerLayoutMap& p): typeSet(t), ptrLayoutMap(p) {}

	void buildPtrLayoutMap();
};

void PtrLayoutMapBuilder::insertMap(const Type* type, const PointerLayout* layout)
{
	ptrLayoutMap.insert(type, layout);
}

const PointerLayout* PtrLayoutMapBuilder::processStructType(StructType* stType)
{
	util::VectorSet<size_t> ptrOffsets;

	auto structLayout = typeSet.getDataLayout().getStructLayout(stType);
	for (unsigned i = 0, e = stType->getNumElements(); i != e; ++i)
	{
		auto offset = structLayout->getElementOffset(i);
		auto subType = stType->getElementType(i);
		auto subLayout = processType(subType);

		for (auto subOffset: *subLayout)
			ptrOffsets.insert(subOffset + offset);
	}

	auto stPtrLayout = PointerLayout::getLayout(std::move(ptrOffsets));
	insertMap(stType, stPtrLayout);
	return stPtrLayout;
}

const PointerLayout* PtrLayoutMapBuilder::processArrayType(ArrayType* arrayType)
{
	auto layout = processType(arrayType->getElementType());
	insertMap(arrayType, layout);
	return layout;
}

const PointerLayout* PtrLayoutMapBuilder::processPointerType(Type* ptrType)
{
	auto layout = PointerLayout::getSinglePointerLayout();
	insertMap(ptrType, layout);
	return layout;
}

const PointerLayout* PtrLayoutMapBuilder::processNonPointerType(Type* nonPtrType)
{
	auto layout = PointerLayout::getEmptyLayout();
	insertMap(nonPtrType, layout);
	return layout;
}


const PointerLayout* PtrLayoutMapBuilder::processType(Type* type)
{
	auto layout = ptrLayoutMap.lookup(type);
	if (layout != nullptr)
		return layout;

	if (auto stType = dyn_cast<StructType>(type))
		return processStructType(stType);
	else if (auto arrayType = dyn_cast<ArrayType>(type))
		return processArrayType(arrayType);
	else if (type->isPointerTy() || type->isFunctionTy())
		return processPointerType(type);
	else
		return processNonPointerType(type);
}

void PtrLayoutMapBuilder::buildPtrLayoutMap()
{
	for (auto type: typeSet)
		processType(type);
}

class PtrLayoutMapPropagator
{
private:
	const CastMap& castMap;
	PointerLayoutMap& ptrLayoutMap;
public:
	PtrLayoutMapPropagator(const CastMap& c, PointerLayoutMap& p): castMap(c), ptrLayoutMap(p) {}

	void propagatePtrLayoutMap();
};

void PtrLayoutMapPropagator::propagatePtrLayoutMap()
{
	for (auto const& mapping: castMap)
	{
		auto lhs = mapping.first;
		auto dstLayout = ptrLayoutMap.lookup(lhs);
		assert(dstLayout != nullptr && "Cannot find ptrLayout for lhs type");

		for (auto rhs: mapping.second)
		{
			auto srcLayout = ptrLayoutMap.lookup(rhs);
			assert(srcLayout != nullptr && "Cannot find ptrLayout for src type");
			dstLayout = PointerLayout::merge(dstLayout, srcLayout);
		}
		ptrLayoutMap.insert(lhs, dstLayout);
	}
}

}

PointerLayoutMap PointerLayoutAnalysis::runOnTypes(const TypeSet& typeSet)
{
	PointerLayoutMap ptrLayoutMap;

	PtrLayoutMapBuilder(typeSet, ptrLayoutMap).buildPtrLayoutMap();

	PtrLayoutMapPropagator(castMap, ptrLayoutMap).propagatePtrLayoutMap();

	return ptrLayoutMap;
}

}
