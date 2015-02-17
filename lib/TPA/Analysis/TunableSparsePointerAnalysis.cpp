#include "MemoryModel/Analysis/GlobalPointerAnalysis.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/ModRefAnalysis.h"
#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "PointerAnalysis/DataFlow/DefUseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/Analysis/TunablePointerAnalysisCore.h"
#include "TPA/Analysis/TunableSparsePointerAnalysis.h"
#include "TPA/DataFlow/SparseAnalysisEngine.h"

#include <llvm/IR/DataLayout.h>

using namespace llvm;

namespace tpa
{

TunableSparsePointerAnalysis::TunableSparsePointerAnalysis(): storeManager(pSetManager), env(pSetManager), memo(storeManager) {}
TunableSparsePointerAnalysis::~TunableSparsePointerAnalysis() = default;

void TunableSparsePointerAnalysis::runOnModule(const llvm::Module& module)
{
	auto dataLayout = DataLayout(&module);
	memManager = std::make_unique<MemoryManager>(dataLayout);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, *memManager, storeManager);

	auto initEnvStore = globalAnalysis.runOnModule(module);

	auto extTable = ExternalPointerEffectTable();
	auto builder = SemiSparseProgramBuilder(extTable);
	auto prog = builder.buildSemiSparseProgram(module);

	// TPA initial pass
	TunablePointerAnalysisCore tpaCore(ptrManager, *memManager, storeManager, initEnvStore.first, extTable);
	tpaCore.runOnProgram(prog, initEnvStore.second);

	ModRefAnalysis modRefAnalysis(tpaCore, extTable);
	auto summaryMap = modRefAnalysis.runOnProgram(prog);

	DefUseProgramBuilder duBuilder(tpaCore, summaryMap, extTable);
	auto dug = duBuilder.buildDefUseProgram(prog);

	auto sparseEngine = SparseAnalysisEngine(ptrManager, *memManager, storeManager, callGraph, memo, summaryMap, extTable);
	env = std::move(initEnvStore.first);
	sparseEngine.runOnDefUseProgram(dug, env, std::move(initEnvStore.second));

	for (auto const& mapping: env)
	{
		auto pSet = tpaCore.getPtsSet(mapping.first);
		assert(pSet != nullptr);
		assert(pSet == mapping.second);
	}
}

const PtsSet* TunableSparsePointerAnalysis::getPtsSet(const Value* val) const
{
	assert(val != nullptr);
	assert(val->getType()->isPointerTy());

	val = val->stripPointerCasts();

	auto ptr = ptrManager.getPointer(Context::getGlobalContext(), val);
	assert(ptr != nullptr);
	return getPtsSet(ptr);
}

const PtsSet* TunableSparsePointerAnalysis::getPtsSet(const Pointer* ptr) const
{
	assert(ptr != nullptr);

	auto pSet = env.lookup(ptr);
	assert(pSet != nullptr);

	return pSet;
}

std::vector<const llvm::Function*> TunableSparsePointerAnalysis::getCallTargets(const Context* ctx, const llvm::Instruction* inst) const
{
	auto retVec = std::vector<const llvm::Function*>();

	for (auto const& callTgt: callGraph.getCallTargets(std::make_pair(ctx, inst)))
		retVec.push_back(callTgt.second);

	std::sort(retVec.begin(), retVec.end());
	retVec.erase(std::unique(retVec.begin(), retVec.end()), retVec.end());

	return retVec;
}

}
