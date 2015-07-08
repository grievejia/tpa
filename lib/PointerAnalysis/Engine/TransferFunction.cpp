#include "Context/KLimitContext.h"
#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"
#include "PointerAnalysis/Program/SemiSparseProgram.h"
#include "PointerAnalysis/Support/FunctionContext.h"
#include "PointerAnalysis/Support/ProgramPoint.h"
#include "Util/IO/PointerAnalysis/Printer.h"

#include <llvm/Support/raw_ostream.h>

using namespace context;
using namespace llvm;
using namespace util::io;

namespace tpa
{

static bool checkCallees(const std::vector<const Function*>& callees)
{
	if (callees.size() == 1)
		return true;

	auto numExternal = std::count_if(callees.begin(), callees.end(), [](const Function* f) { return f->isDeclaration(); });
	bool hasInternal = std::any_of(callees.begin(), callees.end(), [](const Function* f) { return !f->isDeclaration(); });

	if (hasInternal && numExternal == 0)
		return true;
	else if (!hasInternal && numExternal == 1)
		return true;
	else
		return false;
}

static inline size_t countPointerArguments(const llvm::Function* f)
{
	size_t ret = 0;
	for (auto& arg: f->args())
	{
		if (arg.getType()->isPointerTy())
			++ret;
	}
	return ret;
};

void TransferFunction::addTopLevelSuccessors(const ProgramPoint& pp, EvalResult& evalResult)
{
	for (auto const succ: pp.getCFGNode()->uses())
		evalResult.addTopLevelSuccessor(ProgramPoint(pp.getContext(), succ));
}

void TransferFunction::addMemLevelSuccessors(const ProgramPoint& pp, EvalResult& evalResult)
{
	for (auto const succ: pp.getCFGNode()->succs())
		evalResult.addMemLevelSuccessor(ProgramPoint(pp.getContext(), succ));
}

bool TransferFunction::evalMemoryAllocation(const context::Context* ctx, const Instruction* inst, const TypeLayout* type, bool isHeap)
{
	auto ptr = globalState.getPointerManager().getOrCreatePointer(ctx, inst);

	auto mem = 
		isHeap?
		globalState.getMemoryManager().allocateHeapMemory(ctx, inst, type) :
		globalState.getMemoryManager().allocateStackMemory(ctx, inst, type);

	return globalState.getEnv().strongUpdate(ptr, PtsSet::getSingletonSet(mem));
}

bool TransferFunction::evalAllocNode(const Context* ctx, const AllocCFGNode& allocNode)
{
	return evalMemoryAllocation(ctx, allocNode.getDest(), allocNode.getAllocTypeLayout(), false);
}

bool TransferFunction::evalCopyNode(const context::Context* ctx, const CopyCFGNode& copyNode)
{
	std::vector<PtsSet> srcPtsSets;
	srcPtsSets.reserve(copyNode.getNumSrc());

	auto& ptrManager = globalState.getPointerManager();
	auto& env = globalState.getEnv();
	for (auto src: copyNode)
	{
		auto srcPtr = ptrManager.getPointer(ctx, src);

		// This must happen in a PHI node, where one operand must be defined after the CopyNode itself. We need to proceed because the operand may depend on the rhs of this CopyNode and if we give up here, the analysis will reach an immature fixpoint
		if (srcPtr == nullptr)
			continue;

		auto pSet = env.lookup(srcPtr);
		if (pSet.empty())
			// Operand not ready
			return false;

		srcPtsSets.emplace_back(pSet);
	}

	auto dstPtr = ptrManager.getOrCreatePointer(ctx, copyNode.getDest());
	return globalState.getEnv().strongUpdate(dstPtr, PtsSet::mergeAll(srcPtsSets));
}

PtsSet TransferFunction::offsetMemory(const MemoryObject* srcObj, size_t offset, bool isArrayRef)
{
	assert(srcObj != nullptr);

	auto resSet = PtsSet::getEmptySet();
	auto& memManager = globalState.getMemoryManager();
	
	// We have two cases here:
	// - For non-array reference, just access the variable with the given offset
	// - For array reference, we have to examine the variables with offset * 0, offset * 1, offset * 2... all the way till the memory region boundary, if the memory object is not known to be an array previously (this may happen if the program contains nonarray-to-array bitcast)
	if (isArrayRef && offset != 0)
	{
		auto objSize = srcObj->getMemoryBlock()->getTypeLayout()->getSize();

		for (unsigned i = 0, e = objSize - srcObj->getOffset(); i < e; i += offset)
		{
			auto offsetObj = memManager.offsetMemory(srcObj, i);
			resSet = resSet.insert(offsetObj);
		}
	}
	else
	{
		auto offsetObj = memManager.offsetMemory(srcObj, offset);
		resSet = resSet.insert(offsetObj);
	}
	return resSet;
}

bool TransferFunction::copyWithOffset(const Pointer* dst, const Pointer* src, size_t offset, bool isArrayRef)
{
	assert(dst != nullptr && src != nullptr);

	auto& env = globalState.getEnv();
	auto& memManager = globalState.getMemoryManager();

	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return false;

	auto resSet = PtsSet::getEmptySet();
	for (auto srcObj: srcSet)
	{
		// For unknown object, we need to return an unknown to the user. For null object, skip it for now
		// TODO: report this to the user
		if (srcObj == memManager.getNullObject())
			continue;
		else if (srcObj == memManager.getUniversalObject())
		{
			resSet = resSet.insert(srcObj);
			continue;
		}

		resSet = resSet.merge(offsetMemory(srcObj, offset, isArrayRef));
	}

	// For now let's assume that if srcSet contains only null loc, the result should be a universal loc
	if (resSet.empty())
		resSet = PtsSet::getSingletonSet(memManager.getUniversalObject());

	return env.strongUpdate(dst, resSet);
}

bool TransferFunction::evalOffsetNode(const context::Context* ctx, const OffsetCFGNode& offsetNode)
{
	auto& ptrManager = globalState.getPointerManager();

	auto srcPtr = ptrManager.getPointer(ctx, offsetNode.getSrc());
	if (srcPtr == nullptr)
		return false;
	auto dstPtr = ptrManager.getOrCreatePointer(ctx, offsetNode.getDest());

	return copyWithOffset(dstPtr, srcPtr, offsetNode.getOffset(), offsetNode.isArrayRef());
}

PtsSet TransferFunction::loadFromPointer(const Pointer* ptr, const Store& store)
{
	assert(ptr != nullptr);

	auto srcSet = globalState.getEnv().lookup(ptr);
	if (srcSet.empty())
		return srcSet;

	std::vector<PtsSet> srcSets;
	srcSets.reserve(srcSet.size());
	for (auto obj: srcSet)
	{
		auto objSet = store.lookup(obj);
		if (!objSet.empty())
			srcSets.emplace_back(objSet);
	}

	return PtsSet::mergeAll(srcSets);
}

bool TransferFunction::evalLoadNode(const context::Context* ctx, const LoadCFGNode& loadNode, const Store& store)
{
	auto& ptrManager = globalState.getPointerManager();
	auto srcPtr = ptrManager.getPointer(ctx, loadNode.getSrc());
	assert(srcPtr != nullptr && "LoadNode is evaluated before its src operand becomes available");
	auto dstPtr = ptrManager.getOrCreatePointer(ctx, loadNode.getDest());

	auto resSet = loadFromPointer(srcPtr, store);
	return globalState.getEnv().strongUpdate(dstPtr, resSet);
}

void TransferFunction::strongUpdateStore(const MemoryObject* obj, PtsSet pSet, Store& store)
{
	if (!obj->isSpecialObject())
		store.strongUpdate(obj, pSet);
	// TODO: in the else branch, report NULL-pointer dereference to the user
}

void TransferFunction::weakUpdateStore(PtsSet dstSet, PtsSet srcSet, Store& store)
{
	for (auto updateObj: dstSet)
	{
		if (!updateObj->isSpecialObject())
			store.weakUpdate(updateObj, srcSet);
	}
}

bool TransferFunction::evalStore(const Pointer* dst, const Pointer* src, Store& store)
{
	auto& env = globalState.getEnv();

	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return false;

	auto dstSet = env.lookup(dst);
	if (dstSet.empty())
		return false;

	auto dstObj = *dstSet.begin();

	// If the store target is precise and the target location is not unknown
	if (dstSet.size() == 1 && !dstObj->isSummaryObject())
		strongUpdateStore(dstObj, srcSet, store);
	else
		weakUpdateStore(dstSet, srcSet, store);

	return true;
}

bool TransferFunction::evalStoreNode(const context::Context* ctx, const StoreCFGNode& storeNode, Store& store)
{
	auto& ptrManager = globalState.getPointerManager();
	auto srcPtr = ptrManager.getPointer(ctx, storeNode.getSrc());
	auto dstPtr = ptrManager.getPointer(ctx, storeNode.getDest());

	if (srcPtr == nullptr || dstPtr == nullptr)
		return false;

	return evalStore(dstPtr, srcPtr, store);
}

std::vector<const llvm::Function*> TransferFunction::findFunctionInPtsSet(PtsSet pSet, const CallCFGNode& callNode)
{
	auto callees = std::vector<const llvm::Function*>();

	// Two cases here
	if (pSet.has(globalState.getMemoryManager().getUniversalObject()))
	{
		// If funSet contains unknown location, then we can't really derive callees based on the points-to set
		// Instead, guess callees based on the number of arguments
		auto defaultTargets = globalState.getSemiSparseProgram().addr_taken_funcs();
		std::copy_if(
			defaultTargets.begin(),
			defaultTargets.end(),
			std::back_inserter(callees),
			[&callNode] (const Function* f)
			{
				bool isArgMatch = f->isVarArg() || countPointerArguments(f) == callNode.getNumArgument();
				bool isRetMatch = (f->getReturnType()->isPointerTy()) != (callNode.getDest() == nullptr);
				return isArgMatch && isRetMatch;
			}
		);
	}
	else
	{
		for (auto obj: pSet)
		{
			if (obj->isFunctionObject())
				callees.emplace_back(obj->getAllocSite().getFunction());
		}
	}

	return callees;
}

std::vector<const llvm::Function*> TransferFunction::resolveCallTarget(const context::Context* ctx, const CallCFGNode& callNode)
{
	auto callees = std::vector<const llvm::Function*>();

	auto funPtr = globalState.getPointerManager().getPointer(ctx, callNode.getFunctionPointer());
	if (funPtr != nullptr)
	{
		auto funSet = globalState.getEnv().lookup(funPtr);
		if (!funSet.empty())
			callees = findFunctionInPtsSet(funSet, callNode);
	}

	return callees;
}

std::vector<PtsSet> TransferFunction::collectArgumentPtsSets(const context::Context* ctx, const CallCFGNode& callNode, size_t numParams)
{
	std::vector<PtsSet> result;
	result.reserve(numParams);

	auto& ptrManager = globalState.getPointerManager();
	auto& env = globalState.getEnv();
	auto argItr = callNode.begin();
	for (auto i = 0u; i < numParams; ++i)
	{
		auto argPtr = ptrManager.getPointer(ctx, *argItr);
		if (argPtr == nullptr)
			break;

		auto pSet = env.lookup(argPtr);
		if (pSet.empty())
			break;

		result.emplace_back(pSet);
		++argItr;
	}

	return result;
}

bool TransferFunction::updateParameterPtsSets(const FunctionContext& fc, const std::vector<PtsSet>& argSets)
{
	auto changed = false;

	auto& ptrManager = globalState.getPointerManager();
	auto& env = globalState.getEnv();
	auto newCtx = fc.getContext();
	auto paramItr = fc.getFunction()->arg_begin();
	for (auto pSet: argSets)
	{
		assert(paramItr != fc.getFunction()->arg_end());
		while (!paramItr->getType()->isPointerTy())
		{
			++paramItr;
			assert(paramItr != fc.getFunction()->arg_end());
		}
		const Argument* paramVal = paramItr;
		++paramItr;

		auto paramPtr = ptrManager.getOrCreatePointer(newCtx, paramVal);
		changed |= env.weakUpdate(paramPtr, pSet);
	}

	return changed;
}

bool TransferFunction::evalCallArguments(const context::Context* ctx, const CallCFGNode& callNode, const FunctionContext& fc)
{
	auto numParams = countPointerArguments(fc.getFunction());
	assert(callNode.getNumArgument() >= numParams);
;
	auto argSets = collectArgumentPtsSets(ctx, callNode, numParams);
	if (argSets.size() < numParams)
		return false;

	updateParameterPtsSets(fc, argSets);
	return true;
}

void TransferFunction::evalInternalCall(const context::Context* ctx, const CallCFGNode& callNode, const FunctionContext& fc, EvalResult& evalResult, bool callGraphUpdated)
{
	auto tgtCFG = globalState.getSemiSparseProgram().getCFGForFunction(*fc.getFunction());
	assert(tgtCFG != nullptr);
	auto tgtEntryNode = tgtCFG->getEntryNode();

	if (!evalCallArguments(ctx, callNode, fc) && !callGraphUpdated)
		return;

	evalResult.addMemLevelSuccessor(ProgramPoint(fc.getContext(), tgtEntryNode));
}

void TransferFunction::evalCallNode(const context::Context* ctx, const CallCFGNode& callNode, EvalResult& evalResult)
{
	auto callees = resolveCallTarget(ctx, callNode);

	assert(checkCallees(callees) && "Indirect call into multiple external function is not supported yet");

	for (auto f: callees)
	{
		// Update call graph first
		auto callsite = callNode.getCallSite();
		auto newCtx = KLimitContext::pushContext(ctx, callsite);
		auto callTgt = FunctionContext(newCtx, f);
		bool callGraphUpdated = globalState.getCallGraph().insertEdge(ProgramPoint(ctx, &callNode), callTgt);

		// Check whether f is an external library call
		if (f->isDeclaration())
			evalExternalCall(ctx, callNode, callTgt, evalResult);
		else
			evalInternalCall(ctx, callNode, callTgt, evalResult, callGraphUpdated);
	}
}

std::pair<bool, bool> TransferFunction::evalReturnValue(const Context* ctx, const ReturnCFGNode& retNode, const ProgramPoint& retSite)
{
	assert(retSite.getCFGNode()->isCallNode());
	auto const& callNode = static_cast<const CallCFGNode&>(*retSite.getCFGNode());

	auto retVal = retNode.getReturnValue();
	if (retVal == nullptr)
	{
		// calling a void function
		assert(callNode.getDest() == nullptr && "return an unexpected void");
		return std::make_pair(true, false);
	}

	auto dstVal = callNode.getDest();
	if (dstVal == nullptr)
		// Returned a value, but not used by the caller
		return std::make_pair(true, false);

	auto& ptrManager = globalState.getPointerManager();
	auto retPtr = ptrManager.getPointer(ctx, retVal);
	if (retPtr == nullptr)
		// Return value not ready
		return std::make_pair(false, false);

	auto& env = globalState.getEnv();
	auto resSet = env.lookup(retPtr);
	if (resSet.empty())
		// Return pointer not ready
		return std::make_pair(false, false);

	auto dstPtr = ptrManager.getOrCreatePointer(retSite.getContext(), dstVal);
	return std::make_pair(true, env.weakUpdate(dstPtr, resSet));
}

void TransferFunction::evalReturn(const context::Context* ctx, const ReturnCFGNode& retNode, const ProgramPoint& retSite, EvalResult& evalResult)
{
	bool valid, envChanged;
	std::tie(valid, envChanged) = evalReturnValue(ctx, retNode, retSite);

	if (!valid)
		return;
	if (envChanged)
		addTopLevelSuccessors(retSite, evalResult);
	addMemLevelSuccessors(retSite, evalResult);
}

void TransferFunction::evalReturnNode(const context::Context* ctx, const ReturnCFGNode& retNode, EvalResult& evalResult)
{
	if (retNode.getFunction().getName() == "main")
	{
		// Return from main. Do nothing
		errs() << "Reached program end\n";
		return;
	}

	// Merge back pruned mappings in store
	auto prunedStore = globalState.getStorePruner().lookupPrunedStore(FunctionContext(ctx, &retNode.getFunction()));
	if (prunedStore != nullptr)
		evalResult.getStore().mergeWith(*prunedStore);

	for (auto retSite: globalState.getCallGraph().getCallers(FunctionContext(ctx, &retNode.getFunction())))
		evalReturn(ctx, retNode, retSite, evalResult);
}

EvalResult TransferFunction::eval(const ProgramPoint& pp)
{
	//errs() << "Evaluating " << pp << "\n";
	EvalResult evalResult;

	switch (pp.getCFGNode()->getNodeTag())
	{
		case CFGNodeTag::Entry:
		{
			assert(localState != nullptr);

			auto prunedStore = globalState.getStorePruner().pruneStore(*localState, pp);
			evalResult.setStore(std::move(prunedStore));

			addTopLevelSuccessors(pp, evalResult);
			addMemLevelSuccessors(pp, evalResult);
			break;
		}
		case CFGNodeTag::Alloc:
		{
			auto const& allocNode = static_cast<const AllocCFGNode&>(*pp.getCFGNode());
			if (evalAllocNode(pp.getContext(), allocNode))
				addTopLevelSuccessors(pp, evalResult);
			break;
		}
		case CFGNodeTag::Copy:
		{
			auto const& copyNode = static_cast<const CopyCFGNode&>(*pp.getCFGNode());
			if (evalCopyNode(pp.getContext(), copyNode))
				addTopLevelSuccessors(pp, evalResult);
			break;
		}
		case CFGNodeTag::Offset:
		{
			auto const& offsetNode = static_cast<const OffsetCFGNode&>(*pp.getCFGNode());
			if (evalOffsetNode(pp.getContext(), offsetNode))
				addTopLevelSuccessors(pp, evalResult);
			break;
		}
		case CFGNodeTag::Load:
		{
			if (localState != nullptr)
			{
				auto const& loadNode = static_cast<const LoadCFGNode&>(*pp.getCFGNode());

				evalResult.setStore(*localState);
				auto& store = evalResult.getStore();
				if (evalLoadNode(pp.getContext(), loadNode, store))
					addTopLevelSuccessors(pp, evalResult);
				addMemLevelSuccessors(pp, evalResult);
			}
			break;
		}
		case CFGNodeTag::Store:
		{
			if (localState != nullptr)
			{
				auto const& storeNode = static_cast<const StoreCFGNode&>(*pp.getCFGNode());

				evalResult.setStore(*localState);
				auto& store = evalResult.getStore();
				if (evalStoreNode(pp.getContext(), storeNode, store))
					addMemLevelSuccessors(pp, evalResult);
			}
			break;
		}
		case CFGNodeTag::Call:
		{
			if (localState != nullptr)
			{
				auto const& callNode = static_cast<const CallCFGNode&>(*pp.getCFGNode());

				evalResult.setStore(*localState);
				evalCallNode(pp.getContext(), callNode, evalResult);
			}
			break;
		}
		case CFGNodeTag::Ret:
		{
			auto const& retNode = static_cast<const ReturnCFGNode&>(*pp.getCFGNode());

			if (localState != nullptr)
			{
				evalResult.setStore(*localState);
				evalReturnNode(pp.getContext(), retNode, evalResult);
			}
			break;
		}
	}

	return evalResult;
}

}
