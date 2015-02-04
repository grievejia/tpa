#include "MemoryModel/Analysis/GlobalPointerAnalysis.h"
#include "TPA/Analysis/TunableAliasAnalysis.h"
#include "TPA/ControlFlow/SemiSparseProgramBuilder.h"
#include "TPA/DataFlow/ExternalPointerEffectTable.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"
#include "TPA/DataFlow/StaticCallGraph.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "MemoryModel/Memory/MemoryManager.h"

#include <llvm/IR/DataLayout.h>

using namespace llvm;

namespace tpa
{

TunableAliasAnalysis::TunableAliasAnalysis(): env(pSetManager) {}
TunableAliasAnalysis::~TunableAliasAnalysis() = default;

void TunableAliasAnalysis::runOnModule(const llvm::Module& module)
{
	auto dataLayout = DataLayout(&module);
	memManager = std::make_unique<MemoryManager>(dataLayout);

	auto storeManager = StoreManager(pSetManager);
	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, *memManager, storeManager);

	auto initEnvStore = globalAnalysis.runOnModule(module);
	env = std::move(initEnvStore.first);
	auto initStore = std::move(initEnvStore.second);

	auto extTable = ExternalPointerEffectTable();
	auto builder = SemiSparseProgramBuilder(extTable);
	auto prog = builder.buildSemiSparseProgram(module);

	auto memo = Memo(storeManager);
	auto callGraph = StaticCallGraph();
	auto ptrEngine = PointerAnalysisEngine(ptrManager, *memManager, storeManager, callGraph, memo, extTable);
	ptrEngine.runOnProgram(prog, env, std::move(initStore));
}

AliasResult TunableAliasAnalysis::aliasQuery(const Pointer* ptr0, const Pointer* ptr1)
{
	assert(ptr0 != nullptr && ptr1 != nullptr);
	
	auto pSet0 = env.lookup(ptr0);
	auto pSet1 = env.lookup(ptr1);

	if (pSet0 == nullptr || pSet1 == nullptr)
		return AliasResult::NoAlias;

	if (pSet0->has(memManager->getUniversalLocation()) || pSet1->has(memManager->getUniversalLocation()))
		return AliasResult::MayAlias;

	auto intersectSet = VectorSet<const MemoryLocation*>::intersects(*pSet0, *pSet1);
	if (!intersectSet.empty())
	{
		if (intersectSet.size() == 1)
		{
			if (intersectSet[0] == memManager->getNullLocation())
				return AliasResult::NoAlias;
			else if (pSet0->getSize() == 1 && pSet1->getSize() == 1)
				return AliasResult::MustAlias;
		}
		
		return AliasResult::MayAlias;	
	}
	else
		return AliasResult::NoAlias;
}

AliasResult TunableAliasAnalysis::globalAliasQuery(const llvm::Value* v0, const llvm::Value* v1)
{
	assert(v0 != nullptr && v1 != nullptr);

	if (!v0->getType()->isPointerTy() || !v1->getType()->isPointerTy())
		return AliasResult::NoAlias;

	v0 = v0->stripPointerCasts();
	v1 = v1->stripPointerCasts();

	if (v0 == v1)
		return AliasResult::MustAlias;

	// Lookup the corresponding Pointer
	auto globalCtx = Context::getGlobalContext();
	auto ptr0 = ptrManager.getPointer(globalCtx, v0);
	auto ptr1 = ptrManager.getPointer(globalCtx, v1);
	if (ptr0 == nullptr || ptr1 == nullptr)
		return AliasResult::NoAlias;
	
	return aliasQuery(ptr0, ptr1);
}

}
