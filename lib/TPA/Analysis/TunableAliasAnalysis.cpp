#include "TPA/Analysis/TunableAliasAnalysis.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "MemoryModel/Memory/MemoryManager.h"

#include <llvm/IR/DataLayout.h>

using namespace llvm;

namespace tpa
{

void TunableAliasAnalysis::runOnModule(const llvm::Module& module)
{
	tpaAnalysis.runOnModule(module);
	uLoc = tpaAnalysis.getMemoryManager().getUniversalLocation();
	nLoc = tpaAnalysis.getMemoryManager().getNullLocation();
}

AliasResult TunableAliasAnalysis::checkAlias(const PtsSet* pSet0, const PtsSet* pSet1)
{
	if (pSet0 == nullptr || pSet1 == nullptr)
		return AliasResult::NoAlias;

	if (pSet0->has(uLoc) || pSet1->has(uLoc))
		return AliasResult::MayAlias;

	auto intersectSet = VectorSet<const MemoryLocation*>::intersects(*pSet0, *pSet1);
	if (!intersectSet.empty())
	{
		if (intersectSet.size() == 1)
		{
			if (intersectSet[0] == nLoc)
				return AliasResult::NoAlias;
			else if (pSet0->getSize() == 1 && pSet1->getSize() == 1)
				return AliasResult::MustAlias;
		}
		
		return AliasResult::MayAlias;	
	}
	else
		return AliasResult::NoAlias;
}

AliasResult TunableAliasAnalysis::aliasQuery(const Pointer* ptr0, const Pointer* ptr1)
{
	assert(ptr0 != nullptr && ptr1 != nullptr && uLoc != nullptr && nLoc != nullptr);
	
	auto pSet0 = tpaAnalysis.getPtsSet(ptr0);
	auto pSet1 = tpaAnalysis.getPtsSet(ptr1);

	return checkAlias(pSet0, pSet1);
}

AliasResult TunableAliasAnalysis::globalAliasQuery(const llvm::Value* v0, const llvm::Value* v1)
{
	assert(v0 != nullptr && v1 != nullptr && uLoc != nullptr && nLoc != nullptr);

	if (!v0->getType()->isPointerTy() || !v1->getType()->isPointerTy())
		return AliasResult::NoAlias;

	v0 = v0->stripPointerCasts();
	v1 = v1->stripPointerCasts();

	if (v0 == v1)
		return AliasResult::MustAlias;

	auto pSet0 = tpaAnalysis.getPtsSet(v0);
	auto pSet1 = tpaAnalysis.getPtsSet(v1);
	
	return checkAlias(pSet0, pSet1);
}

}
