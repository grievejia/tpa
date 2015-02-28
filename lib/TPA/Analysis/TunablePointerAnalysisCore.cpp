#include "MemoryModel/Analysis/GlobalPointerAnalysis.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/Analysis/TunablePointerAnalysisCore.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

TunablePointerAnalysisCore::TunablePointerAnalysisCore(PointerManager& p, MemoryManager& m, StoreManager& sm, const Env& e, ExternalPointerEffectTable& ex): ptrManager(p), memManager(m), storeManager(sm), extTable(ex), env(e), memo(storeManager) {}

void TunablePointerAnalysisCore::runOnProgram(const PointerProgram& prog, const Store& store)
{
	auto ptrEngine = PointerAnalysisEngine(ptrManager, memManager, storeManager, callGraph, memo, extTable);
	ptrEngine.runOnProgram(prog, env, store);
}

const PtsSet* TunablePointerAnalysisCore::getPtsSet(const Value* val) const
{
	return getPtsSet(Context::getGlobalContext(), val);
}

const PtsSet* TunablePointerAnalysisCore::getPtsSet(const Context* ctx, const Value* val) const
{
	assert(val != nullptr);
	assert(val->getType()->isPointerTy());

	val = val->stripPointerCasts();

	auto ptr = ptrManager.getPointer(ctx, val);
	assert(ptr != nullptr);
	return getPtsSet(ptr);
}

const PtsSet* TunablePointerAnalysisCore::getPtsSet(const Pointer* ptr) const
{
	assert(ptr != nullptr);

	auto pSet = env.lookup(ptr);
	assert(pSet != nullptr);

	return pSet;
}

std::vector<const llvm::Function*> TunablePointerAnalysisCore::getCallTargets(const Context* ctx, const llvm::Instruction* inst) const
{
	auto retVec = std::vector<const llvm::Function*>();

	ImmutableCallSite cs(inst);
	assert(cs && "getCallTargets() gets a non-call inst");
	if (auto f = cs.getCalledFunction())
		retVec.push_back(f);
	else
	{
		for (auto const& callTgt: callGraph.getCallTargets(std::make_pair(ctx, inst)))
		retVec.push_back(callTgt.second);

		std::sort(retVec.begin(), retVec.end());
		retVec.erase(std::unique(retVec.begin(), retVec.end()), retVec.end());
	}

	return retVec;
}

}