#include "PtsDumpPass.h"
#include "MemoryModel/Memory/MemoryManager.h"
#include "TPA/Analysis/TunablePointerAnalysisWrapper.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

static void dumpValueName(const Value* value)
{
	if (auto inst = dyn_cast<Instruction>(value))
		errs() << inst->getParent()->getParent()->getName() << "::" << inst->getName();
	else if (auto global = dyn_cast<GlobalValue>(value))
		errs() << "[GLOBAL]::" << global->getName();
	else if (auto arg = dyn_cast<Argument>(value))
		errs() << arg->getParent()->getName() << "::" << arg->getName();
	else
	{
		errs() << "Don't know how to dump name for this value: " << *value << "\n";
		llvm_unreachable("");
	}
}

static void dumpPtsSet(PtsSet ptsSet, const MemoryManager& memManager)
{
	errs() << "{  ";
	for (auto loc: ptsSet)
	{
		if (loc == memManager.getUniversalLocation())
			errs() << "(anywhere) ";
		else if (loc == memManager.getNullLocation())
			errs() << "(null) ";
		else
			errs() << *loc << "  ";
	}
	errs() << "}\n";
}

static void dumpPtsSetForValue(const Value* value, const PointerAnalysis& ptrAnalysis)
{
	assert(value != nullptr);
	assert(value->getType()->isPointerTy());

	dumpValueName(value);
	errs() << " -> ";
	dumpPtsSet(ptrAnalysis.getPtsSet(value), ptrAnalysis.getMemoryManager());
}

static void dumpPtsSetInBasicBlock(const BasicBlock& bb, const PointerAnalysis& ptrAnalysis)
{
	for (auto const& inst: bb)
	{
		if (inst.getType()->isPointerTy())
			dumpPtsSetForValue(&inst, ptrAnalysis);
	}
}

static void dumpPtsSetInFunction(const Function& f, const PointerAnalysis& ptrAnalysis)
{
	for (auto const& bb: f)
		dumpPtsSetInBasicBlock(bb, ptrAnalysis);
}

static void dumpPtsSetInModule(const Module& module, const PointerAnalysis& ptrAnalysis)
{
	for (auto const& g: module.globals())
		dumpPtsSetForValue(&g, ptrAnalysis);
	for (auto const& f: module)
		dumpPtsSetInFunction(f, ptrAnalysis);
}

bool PtsDumpPass::runOnModule(Module& module)
{
	TunablePointerAnalysisWrapper tpaWrapper;
	tpaWrapper.runOnModule(module);

	auto const& ptrAnalysis = tpaWrapper.getPointerAnalysis();

	dumpPtsSetInModule(module, ptrAnalysis);

	return false;
}

void PtsDumpPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char PtsDumpPass::ID = 0;