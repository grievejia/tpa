#include "PointerAnalysis/FrontEnd/Type/StructCastAnalysis.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>

using namespace llvm;

namespace llvm
{

class BitCastOperator: public ConcreteOperator<Operator, Instruction::BitCast>
{
	friend class BitCastInst;
	friend class ConstantExpr;
public:
	Type* getSrcTy() const
	{
		return getOperand(0)->getType();
	}

	Type* getDestTy() const
	{
		return getType();
	}
};

}

namespace tpa
{

namespace
{

class CastMapBuilder
{
private:
	const Module& module;
	CastMap& structCastMap;

	void collectCast(const Value&, CastMap&);
	CastMap collectAllCasts();
	void computeTransitiveClosure(CastMap&);
	void extractStructs(CastMap&);
public:
	CastMapBuilder(const Module& m, CastMap& c): module(m), structCastMap(c) {}

	void buildCastMap();
};

void CastMapBuilder::collectCast(const Value& value, CastMap& castMap)
{
	if (auto bc = dyn_cast<BitCastOperator>(&value))
	{
		auto srcType = bc->getSrcTy();
		auto dstType = bc->getDestTy();

		if (!srcType->isPointerTy() || !dstType->isPointerTy())
			return;

		// Sometimes, a value is casted only to satisfy the type signature of a particular API (e.g. llvm.memcpy). Exclude those cases
		if (bc->hasOneUse())
		{
			auto user = *bc->user_begin();
			if (isa<MemIntrinsic>(user))
				return;
		}

		castMap.insert(srcType, dstType);
	}
}

CastMap CastMapBuilder::collectAllCasts()
{
	CastMap castMap;

	for (auto const& global: module.globals())
	{
		if (global.hasInitializer())
			collectCast(*global.getInitializer(), castMap);
	}

	for (auto const& f: module)
		for (auto const& bb: f)
			for (auto const& inst: bb)
				collectCast(inst, castMap);

	return castMap;
}

void CastMapBuilder::computeTransitiveClosure(CastMap& castMap)
{
	// This is a really naive and inefficient way of computing transitive closure. However typically the size of the map won't be too large, hence such a method is very often acceptable. We'll try to optimize it once it becomes a problem
	bool changed;
	do
	{
		changed = false;
		for (auto& mapping: castMap)
		{
			// We need to make a copy here
			auto types = mapping.second;
			for (auto type: types)
			{
				auto itr = castMap.find(type);
				if (itr != castMap.end())
				{
					for (auto dstType: itr->second)
					{
						if (dstType != mapping.first)
							changed |= mapping.second.insert(dstType).second;
					}
				}
			}
		}
	} while (changed);
}

void CastMapBuilder::extractStructs(CastMap& castMap)
{
	for (auto const& mapping: castMap)
	{
		auto lhs = mapping.first->getPointerElementType();
		if (!lhs->isStructTy())
			continue;
		
		auto& rhsSet = structCastMap.getOrCreateRHS(lhs);
		for (auto dstType: mapping.second)
		{
			auto rhs = dstType->getPointerElementType();
			if (!rhs->isStructTy())
				continue;
			rhsSet.insert(rhs);
		}
	}
}

void CastMapBuilder::buildCastMap()
{
	auto allCastMap = collectAllCasts();
	computeTransitiveClosure(allCastMap);
	extractStructs(allCastMap);
}

}

CastMap StructCastAnalysis::runOnModule(const Module& module)
{
	CastMap structCastMap;

	CastMapBuilder(module, structCastMap).buildCastMap();

	return structCastMap;
}

}