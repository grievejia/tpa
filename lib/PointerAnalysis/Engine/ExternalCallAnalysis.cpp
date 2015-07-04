#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Annotation/Pointer/PointerEffect.h"
#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;

namespace tpa
{

static const Value* getArgument(const CallCFGNode& callNode, const APosition& pos)
{
	auto inst = callNode.getCallSite();
	if (pos.isReturnPosition())
		return inst;

	// We can't just call callNode.getArgument(...) because there might be non-pointer args that are not included in callNode
	ImmutableCallSite cs(inst);
	assert(cs);
	
	auto argIdx = pos.getAsArgPosition().getArgIndex();
	assert(cs.arg_size() > argIdx);

	return cs.getArgument(argIdx)->stripPointerCasts();
}

bool TransferFunction::evalExternalAlloc(const context::Context* ctx, const CallCFGNode& callNode, const PointerAllocEffect& allocEffect)
{
	// TODO: add type hint to malloc-like calls
	auto dstVal = callNode.getDest();
	if (dstVal == nullptr)
		return false;

	return evalMemoryAllocation(ctx, dstVal, TypeLayout::getByteArrayTypeLayout(), true);
}

void TransferFunction::evalMemcpyPtsSet(const MemoryObject* dstObj, const std::vector<const MemoryObject*>& srcObjs, size_t startingOffset, Store& store)
{
	auto& memManager = globalState.getMemoryManager();
	for (auto srcObj: srcObjs)
	{
		auto srcSet = store.lookup(srcObj);
		if (srcSet.empty())
			continue;

		auto offset = srcObj->getOffset() - startingOffset;
		auto tgtObj = memManager.offsetMemory(dstObj, offset);
		if (tgtObj->isSpecialObject())
			break;
		store.weakUpdate(tgtObj, srcSet);
	}
}

bool TransferFunction::evalMemcpyPointer(const Pointer* dst, const Pointer* src, Store& store)
{
	auto& env = globalState.getEnv();

	auto dstSet = env.lookup(dst);
	if (dstSet.empty())
		return false;
	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return false;

	auto& memManager = globalState.getMemoryManager();
	for (auto srcObj: srcSet)
	{
		auto srcObjs = memManager.getReachablePointerObjects(srcObj);
		for (auto dstObj: dstSet)
			evalMemcpyPtsSet(dstObj, srcObjs, srcObj->getOffset(), store);
	}
	return true;
}

bool TransferFunction::evalMemcpy(const context::Context* ctx, const CallCFGNode& callNode, Store& store, const APosition& dstPos, const APosition& srcPos)
{
	assert(dstPos.isArgPosition() && srcPos.isArgPosition() && "memcpy only operates on arguments");

	auto& ptrManager = globalState.getPointerManager();
	auto dstPtr = ptrManager.getPointer(ctx, getArgument(callNode, dstPos));
	if (dstPtr == nullptr)
		return false;
	auto srcPtr = ptrManager.getPointer(ctx, getArgument(callNode, srcPos));
	if (srcPtr == nullptr)
		return false;

	return evalMemcpyPointer(dstPtr, srcPtr, store);
}

PtsSet TransferFunction::evalExternalCopySource(const context::Context* ctx, const CallCFGNode& callNode, Store& store, const CopySource& src)
{
	switch (src.getType())
	{
		case CopySource::SourceType::Value:
		{
			auto ptr = globalState.getPointerManager().getPointer(ctx, getArgument(callNode, src.getPosition()));
			if (ptr == nullptr)
				return PtsSet::getEmptySet();
			return globalState.getEnv().lookup(ptr);
		}
		case CopySource::SourceType::DirectMemory:
		{
			auto ptr = globalState.getPointerManager().getPointer(ctx, getArgument(callNode, src.getPosition()));
			if (ptr == nullptr)
				return PtsSet::getEmptySet();
			return loadFromPointer(ptr, store);
		}
		case CopySource::SourceType::Universal:
		{
			return PtsSet::getSingletonSet(globalState.getMemoryManager().getUniversalObject());
		}
		case CopySource::SourceType::Null:
		{
			return PtsSet::getSingletonSet(globalState.getMemoryManager().getNullObject());
		}
		case CopySource::SourceType::Static:
			// TODO: model "static" memory
			return PtsSet::getSingletonSet(globalState.getMemoryManager().getUniversalObject());
		case CopySource::SourceType::ReachableMemory:
		{
			llvm_unreachable("ReachableMemory src should be handled earlier");
		}
	}
}

void TransferFunction::fillPtsSetWith(const Pointer* ptr, PtsSet srcSet, Store& store)
{
	auto pSet = globalState.getEnv().lookup(ptr);
	
	for (auto obj: pSet)
	{
		if (obj->isSpecialObject())
			continue;

		auto candidateObjs = globalState.getMemoryManager().getReachablePointerObjects(obj);
		for (auto tgtObj: candidateObjs)
			store.weakUpdate(tgtObj, srcSet);
	}
}

std::pair<bool, bool> TransferFunction::evalExternalCopyDest(const context::Context* ctx, const CallCFGNode& callNode, Store& store, const CopyDest& dest, PtsSet srcSet)
{
	// If the return value is not used, don't bother process it
	if (callNode.getDest() == nullptr && dest.getPosition().isReturnPosition())
		return std::make_pair(false, true);

	auto dstPtr = globalState.getPointerManager().getOrCreatePointer(ctx, getArgument(callNode, dest.getPosition()));
	switch (dest.getType())
	{
		case CopyDest::DestType::Value:
		{
			auto envChanged = globalState.getEnv().weakUpdate(dstPtr, srcSet);
			return std::make_pair(envChanged, true);
		}
		case CopyDest::DestType::DirectMemory:
		{
			auto dstSet = globalState.getEnv().lookup(dstPtr);
			if (dstSet.empty())
				return std::make_pair(false, false);

			weakUpdateStore(dstSet, srcSet, store);
			return std::make_pair(false, true);
		}
		case CopyDest::DestType::ReachableMemory:
		{
			fillPtsSetWith(dstPtr, srcSet, store);
			return std::make_pair(false, true);
		}
	}
}

std::pair<bool, bool> TransferFunction::evalExternalCopy(const context::Context* ctx, const CallCFGNode& callNode, Store& store, const PointerCopyEffect& copyEffect)
{
	auto const& src = copyEffect.getSource();
	auto const& dest = copyEffect.getDest();

	// Special case for memcpy: the source is not a single ptr/mem
	if (src.getType() == CopySource::SourceType::ReachableMemory)
	{
		assert(dest.getType() == CopyDest::DestType::ReachableMemory && "R src can only be assigned to R dest");
		auto succ = evalMemcpy(ctx, callNode, store, dest.getPosition(), src.getPosition());
		return std::make_pair(false, succ);
	}

	auto srcSet = evalExternalCopySource(ctx, callNode, store, src);
	if (srcSet.empty())
		return std::make_pair(false, false);

	return evalExternalCopyDest(ctx, callNode, store, dest, srcSet);
}

void TransferFunction::evalExternalCallByEffect(const context::Context* ctx, const CallCFGNode& callNode, const PointerEffect& effect, EvalResult& evalResult)
{
	switch (effect.getType())
	{
		case PointerEffectType::Alloc:
		{
			if (evalExternalAlloc(ctx, callNode, effect.getAsAllocEffect()))
				addTopLevelSuccessors(ProgramPoint(ctx, &callNode), evalResult);
			break;
		}
		case PointerEffectType::Copy:
		{
			bool enqueueTop, enqueueMem;
			std::tie(enqueueTop, enqueueMem) = evalExternalCopy(ctx, callNode, evalResult.getStore(), effect.getAsCopyEffect());
			if (enqueueTop)
				addTopLevelSuccessors(ProgramPoint(ctx, &callNode), evalResult);
			if (enqueueMem)
				addMemLevelSuccessors(ProgramPoint(ctx, &callNode), evalResult);
			break;
		}
		case PointerEffectType::Exit:
			break;
	}
}

void TransferFunction::evalExternalCall(const context::Context* ctx, const CallCFGNode& callNode, const FunctionContext& fc, EvalResult& evalResult)
{
	auto summary = globalState.getExternalPointerTable().lookup(fc.getFunction()->getName());
	if (summary == nullptr)
	{
		errs() << "\nPointer Analysis error: cannot find annotation for the following function:\n" << fc.getFunction()->getName() << "\n\n";
		errs() << "Please add annotation to the aforementioned function and in the config file and try again.\n";
		std::exit(-1);
	}

	for (auto const& effect: *summary)
		evalExternalCallByEffect(ctx, callNode, effect, evalResult);
}

}