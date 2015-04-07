#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/PtsSet/PtsSet.h"
#include "PointerAnalysis/Analysis/AliasAnalysis.h"

using namespace llvm;

namespace tpa
{

AliasAnalysis::AliasAnalysis(const PointerAnalysis& p): ptrAnalysis(p), uLoc(ptrAnalysis.getMemoryManager().getUniversalLocation()), nLoc(ptrAnalysis.getMemoryManager().getNullLocation()) {}

AliasResult AliasAnalysis::checkAlias(PtsSet pSet0, PtsSet pSet1)
{
	if (pSet0.isEmpty()|| pSet1.isEmpty())
		return AliasResult::NoAlias;

	if (pSet0.has(uLoc) || pSet1.has(uLoc))
		return AliasResult::MayAlias;

	auto intersectSet = PtsSet::intersects(pSet0, pSet1);
	if (!intersectSet.empty())
	{
		if (intersectSet.size() == 1)
		{
			if (intersectSet[0] == nLoc)
				return AliasResult::NoAlias;
			else if (pSet0.getSize() == 1 && pSet1.getSize() == 1)
				return AliasResult::MustAlias;
		}
		
		return AliasResult::MayAlias;	
	}
	else
		return AliasResult::NoAlias;
}

AliasResult AliasAnalysis::aliasQuery(const Pointer* ptr0, const Pointer* ptr1)
{
	assert(ptr0 != nullptr && ptr1 != nullptr);
	
	auto pSet0 = ptrAnalysis.getPtsSet(ptr0);
	auto pSet1 = ptrAnalysis.getPtsSet(ptr1);

	return checkAlias(pSet0, pSet1);
}

AliasResult AliasAnalysis::globalAliasQuery(const llvm::Value* v0, const llvm::Value* v1)
{
	assert(v0 != nullptr && v1 != nullptr);

	if (!v0->getType()->isPointerTy() || !v1->getType()->isPointerTy())
		return AliasResult::NoAlias;

	v0 = v0->stripPointerCasts();
	v1 = v1->stripPointerCasts();

	if (v0 == v1)
		return AliasResult::MustAlias;

	auto pSet0 = ptrAnalysis.getPtsSet(v0);
	auto pSet1 = ptrAnalysis.getPtsSet(v1);
	
	return checkAlias(pSet0, pSet1);
}

}