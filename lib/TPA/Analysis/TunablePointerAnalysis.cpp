#include "MemoryModel/Analysis/GlobalPointerAnalysis.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"
#include "TPA/DataFlow/PointerAnalysisEngine.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

TunablePointerAnalysis::TunablePointerAnalysis(): storeManager(pSetManager), env(pSetManager), memo(storeManager) {}
TunablePointerAnalysis::~TunablePointerAnalysis() = default;

void TunablePointerAnalysis::runOnModule(const llvm::Module& module)
{
	auto dataLayout = DataLayout(&module);
	memManager = std::make_unique<MemoryManager>(dataLayout);

	auto globalAnalysis = GlobalPointerAnalysis(ptrManager, *memManager, storeManager);

	auto initEnvStore = globalAnalysis.runOnModule(module);
	env = std::move(initEnvStore.first);
	auto initStore = std::move(initEnvStore.second);

	auto extTable = ExternalPointerEffectTable();
	auto builder = SemiSparseProgramBuilder(extTable);
	prog = builder.buildSemiSparseProgram(module);

	auto ptrEngine = PointerAnalysisEngine(ptrManager, *memManager, storeManager, callGraph, memo, extTable);
	ptrEngine.runOnProgram(prog, env, std::move(initStore));

	//env.dump(errs());
	auto ctxs = Context::getAllContexts();
	for (auto ctx: ctxs)
		errs() << "\t\t" << *ctx << "\n";
	errs() << "\tTotal # of contexts = " << ctxs.size() << "\n";
}

const PtsSet* TunablePointerAnalysis::getPtsSet(const Value* val) const
{
	auto ctxs = Context::getAllContexts();
	auto retSet = pSetManager.getEmptySet();
	for (auto ctx: ctxs)
	{
		auto newSet = getPtsSet(ctx, val);
		if (newSet != nullptr)
			retSet = pSetManager.mergeSet(retSet, newSet);
	}
	return retSet;
}

const PtsSet* TunablePointerAnalysis::getPtsSet(const Context* ctx, const llvm::Value* val) const
{
	assert(val != nullptr);
	assert(val->getType()->isPointerTy());

	val = val->stripPointerCasts();

	auto ptr = ptrManager.getPointer(ctx, val);
	if (ptr == nullptr)
		return nullptr;

	return getPtsSet(ptr);
}

const PtsSet* TunablePointerAnalysis::getPtsSet(const Pointer* ptr) const
{
	assert(ptr != nullptr);

	auto pSet = env.lookup(ptr);
	assert(pSet != nullptr);

	return pSet;
}

std::vector<const llvm::Function*> TunablePointerAnalysis::getCallTargets(const Context* ctx, const llvm::Instruction* inst) const
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
