#include "PointerAnalysis/FrontEnd/Type/ArrayLayoutAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/TypeSet.h"
#include "PointerAnalysis/MemoryModel/Type/ArrayLayout.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

using namespace llvm;

namespace tpa
{

namespace
{

class ArrayLayoutMapBuilder
{
private:
	const TypeSet& typeSet;
	ArrayLayoutMap& arrayLayoutMap;

	void insertMap(const Type*, const ArrayLayout*);

	const ArrayLayout* processStructType(StructType*);
	const ArrayLayout* processArrayType(ArrayType*);
	const ArrayLayout* processSimpleType(Type*);
	const ArrayLayout* processType(Type*);
public:
	ArrayLayoutMapBuilder(const TypeSet& t, ArrayLayoutMap& a): typeSet(t), arrayLayoutMap(a) {}

	void buildArrayLayoutMap();
};

void ArrayLayoutMapBuilder::insertMap(const Type* type, const ArrayLayout* layout)
{
	arrayLayoutMap.insert(type, layout);
}

const ArrayLayout* ArrayLayoutMapBuilder::processStructType(StructType* stType)
{
	// We know nothing about opaque type. Conservatively treat it as a byte array
	if (stType->isOpaque())
	{
		auto layout = ArrayLayout::getByteArrayLayout();
		insertMap(stType, layout);
		return layout;
	}

	ArrayLayout::ArrayTripleList arrayTripleList;

	auto structLayout = typeSet.getDataLayout().getStructLayout(stType);
	for (unsigned i = 0, e = stType->getNumElements(); i != e; ++i)
	{
		auto offset = structLayout->getElementOffset(i);

		auto subType = stType->getElementType(i);
		auto subLayout = processType(subType);
		if (!subLayout->empty())
		{
			// Adjust the start and end field in the sub-layout according to the offset
			std::transform(
				subLayout->begin(),
				subLayout->end(),
				std::back_inserter(arrayTripleList),
				[offset] (auto const& triple) -> ArrayTriple
				{
					return { triple.start + offset, triple.end + offset, triple.size };
				}
			);
		}
	}

	auto stArrayLayout = ArrayLayout::getLayout(std::move(arrayTripleList));
	insertMap(stType, stArrayLayout);
	return stArrayLayout;
}

const ArrayLayout* ArrayLayoutMapBuilder::processArrayType(ArrayType* arrayType)
{
	auto elemType = arrayType->getElementType();
	auto numElems = arrayType->getNumElements();
	auto elemSize = typeSet.getDataLayout().getTypeAllocSize(elemType);

	auto arraySize = numElems * elemSize;
	ArrayLayout::ArrayTripleList arrayTripleList = {{0, arraySize, elemSize}};
	// We need to examine the element type here because it may contain additional array triple
	auto subLayout = processType(elemType);
	if (!subLayout->empty())
		arrayTripleList.insert(arrayTripleList.end(), subLayout->begin(), subLayout->end());

	// Remove duplicate entries in triple list
	// TODO: arrays with size 1 are not "real" arrays. Remove them
	arrayTripleList.erase(std::unique(arrayTripleList.begin(), arrayTripleList.end()), arrayTripleList.end());

	auto arrayLayout = ArrayLayout::getLayout(std::move(arrayTripleList));
	insertMap(arrayType, arrayLayout);
	return arrayLayout;
}

const ArrayLayout* ArrayLayoutMapBuilder::processSimpleType(Type* simpleType)
{
	auto layout = ArrayLayout::getDefaultLayout();
	insertMap(simpleType, layout);
	return layout;
}

const ArrayLayout* ArrayLayoutMapBuilder::processType(Type* type)
{
	auto layout = arrayLayoutMap.lookup(type);
	if (layout != nullptr)
		return layout;

	if (auto stType = dyn_cast<StructType>(type))
		return processStructType(stType);
	else if (auto arrayType = dyn_cast<ArrayType>(type))
		return processArrayType(arrayType);
	else
		return processSimpleType(type);
}

void ArrayLayoutMapBuilder::buildArrayLayoutMap()
{
	for (auto type: typeSet)
		processType(type);
}

}

ArrayLayoutMap ArrayLayoutAnalysis::runOnTypes(const TypeSet& typeSet)
{
	ArrayLayoutMap arrayLayoutMap;

	ArrayLayoutMapBuilder(typeSet, arrayLayoutMap).buildArrayLayoutMap();

	return arrayLayoutMap;
}

}
