#include "RunAnalysis.h"

#include "Context/KLimitContext.h"
#include "Dynamic/Analysis/DynamicContext.h"
#include "Dynamic/Analysis/DynamicPointerAnalysis.h"
#include "Dynamic/Instrument/IDAssigner.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/SemiSparseProgramBuilder.h"
#include "Util/IO/PointerAnalysis/Printer.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace dynamic;
using namespace llvm;
using namespace util::io;

namespace
{

void dumpContext(const DynamicContext* ctx)
{
	if (ctx->getDepth() == 0)
	{
		outs() << "(GLOBAL)";
		return;
	}

	auto currCtx = ctx;
	outs() << "(";
	while (currCtx->getDepth() > 0)
	{
		outs() << currCtx->getCallSite();
		currCtx = DynamicContext::popContext(currCtx);
	}
	outs() << ")";
}

void dumpValue(const Value* val)
{
	if (isa<GlobalValue>(val))
		outs() << val->getName();
	else if (isa<Function>(val))
		outs() << val->getName();
	else if (auto arg = dyn_cast<Argument>(val))
	{
		auto func = arg->getParent();
		outs() << func->getName() << "/" << arg->getName();
	}
	else if (auto inst = dyn_cast<Instruction>(val))
	{
		auto func = inst->getParent()->getParent();
		outs() << func->getName() << "/" << inst->getName();
	}
	else
		outs() << *val;
}

void dumpID(unsigned id, const IDAssigner& idMap)
{
	if (id == 0)
		outs() << "null";
	else
	{
		auto val = idMap.getValue(id);
		assert(val != nullptr && "Illegal ID");
		dumpValue(val);
	}
}

void dumpPointer(const DynamicPointer& ptr, const IDAssigner& idMap)
{
	outs() << "<";
	dumpContext(ptr.getContext());
	outs() << "::";
	dumpID(ptr.getID(), idMap);
	outs() << ">";
}

void dumpMemoryObject(const DynamicMemoryObject& obj, const IDAssigner& idMap)
{
	outs() << "[";
	dumpPointer(obj.getAllocSite(), idMap);
	outs() << " + ";
	outs() << obj.getOffset() << "]";
}

void dumpDynamicPtsMap(const DynamicPointerAnalysis& dynAnalysis, const IDAssigner& idMap)
{
	for (auto const& mapping: dynAnalysis)
	{
		auto const& ptr = mapping.first;
		auto const& obj = mapping.second;

		dumpPointer(ptr, idMap);
		outs() << "  -->>  ";
		dumpMemoryObject(obj, idMap);
		outs() << "\n";
	}
}

const context::Context* translateContext(const DynamicContext* dynCtx, const IDAssigner& idMap)
{
	static std::unordered_map<const DynamicContext*, const context::Context*> ctxMap;
	auto itr = ctxMap.find(dynCtx);
	if (itr != ctxMap.end())
		return itr->second;

	std::vector<const Instruction*> callsites;
	callsites.reserve(dynCtx->getDepth());
	auto currDynCtx = dynCtx;
	while (currDynCtx->getDepth() > 0)
	{
		auto csValue = idMap.getValue(currDynCtx->getCallSite());
		assert(csValue != nullptr);

		callsites.push_back(cast<Instruction>(csValue));
		currDynCtx = DynamicContext::popContext(currDynCtx);
	}

	assert(callsites.size() <= dynCtx->getDepth());

	auto ctx = context::Context::getGlobalContext();
	for (auto itr = callsites.rbegin(), ite = callsites.rend(); itr != ite; ++itr)
	{
		ctx = context::KLimitContext::pushContext(ctx, *itr);
	}

	assert(ctx->size() <= dynCtx->getDepth());

	ctxMap.insert(itr, std::make_pair(dynCtx, ctx));
	return ctx;
}

const tpa::Pointer* translatePointer(const DynamicPointer& dynPtr, const IDAssigner& idMap, const tpa::PointerManager& ptrManager)
{
	auto ctx = translateContext(dynPtr.getContext(), idMap);
	assert(dynPtr.getID() != 0);
	auto ptrValue = idMap.getValue(dynPtr.getID());
	assert(ptrValue != nullptr);
	assert(ptrValue->getType()->isPointerTy());

	auto ptr = ptrManager.getPointer(ctx, ptrValue);
	if (ptr == nullptr)
	{
		errs() << "Missing pointer\n";
		errs() << "\tContext = " << *ctx << ", value = " << *ptrValue << "\n";
	}
	return ptr;
}

tpa::AllocSite translateMemory(const DynamicMemoryObject& dynObj, const IDAssigner& idMap)
{
	if (dynObj.getAllocSite().getID() == 0)
		return tpa::AllocSite::getNullAllocSite();

	auto ctx = translateContext(dynObj.getAllocSite().getContext(), idMap);
	auto ptrValue = idMap.getValue(dynObj.getAllocSite().getID());
	assert(ptrValue != nullptr);

	if (auto func = dyn_cast<Function>(ptrValue))
	{
		return tpa::AllocSite::getFunctionAllocSite(func);
	}
	else if (auto gv = dyn_cast<GlobalVariable>(ptrValue))
	{
		return tpa::AllocSite::getGlobalAllocSite(gv);
	}
	else if (isa<AllocaInst>(ptrValue) || isa<Argument>(ptrValue))
	{
		return tpa::AllocSite::getStackAllocSite(ctx, ptrValue);
	}
	else
	{
		return tpa::AllocSite::getHeapAllocSite(ctx, ptrValue);
	}
}

template <typename T>
bool checkPointerAnalysis(const tpa::PointerAnalysis<T>& ptrAnalysis, const tpa::Pointer* ptr, const tpa::AllocSite& allocSite)
{
	auto pSet = ptrAnalysis.getPtsSet(ptr);
	if (pSet.has(tpa::MemoryManager::getUniversalObject()))
		return true;

	for (auto obj: pSet)
	{
		if (obj->getAllocSite() == allocSite)
			return true;
	}

	errs() << "Unsound points-to set found\n";
	errs() << "\tPointer: " << *ptr << "\n";
	return false;
}

template <typename T>
bool checkResult(const DynamicPointerAnalysis& dynAnalysis, const tpa::PointerAnalysis<T>& ptrAnalysis, const IDAssigner& idMap)
{
	bool passed = true;
	for (auto const& mapping: dynAnalysis)
	{
		auto& ptr = mapping.first;
		auto& obj = mapping.second;

		auto tpaPtr = translatePointer(ptr, idMap, ptrAnalysis.getPointerManager());
		if (tpaPtr == nullptr)
		{
			passed = false;
			continue;
		}

		auto tpaAllocSite = translateMemory(obj, idMap);

		passed &= checkPointerAnalysis(ptrAnalysis, tpaPtr, tpaAllocSite);
	}

	return passed;
}

}

bool runAnalysisOnModule(const Module& module, const char* logName, const char* configName, unsigned k)
{
	outs() << "Step1> Rebuilding ID map from the input module...\n\n";
	auto idMap = IDAssigner(module);

	outs() << "Step2> Running dynamic analysis on log " << logName << "...\n\n";
	auto dynAnalysis = DynamicPointerAnalysis(logName);
	dynAnalysis.process();
	//dumpDynamicPtsMap(dynAnalysis, idMap);

	outs() << "Step3> Running pointer analysis with k=" << k << " ...\n\n";
	auto ptrAnalysis = tpa::SemiSparsePointerAnalysis();
	ptrAnalysis.loadExternalPointerTable(configName);
	tpa::SemiSparseProgramBuilder ssProgBuilder;
	auto ssProg = ssProgBuilder.runOnModule(module);
	context::KLimitContext::setLimit(k);
	ptrAnalysis.runOnProgram(ssProg);

	outs() << "Step4> Checking the result of pointer analysis...\n\n";
	return checkResult(dynAnalysis, ptrAnalysis, idMap);
}
