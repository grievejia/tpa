#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"

namespace tpa
{

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
		//errs() << "obj = " << *srcObj << ", size = " << objSize - srcObj->getOffset() << "\n";

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
	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return false;

	std::vector<PtsSet> srcPtsSets;
	srcPtsSets.reserve(srcSet.size());

	for (auto srcObj: srcSet)
	{
		// For unknown object, we need to return an unknown to the user. For null object, skip it for now
		// TODO: report this to the user
		if (srcObj->isNullObject())
			continue;
		else if (srcObj->isUniversalObject())
		{
			srcPtsSets.emplace_back(PtsSet::getSingletonSet(MemoryManager::getUniversalObject()));
			break;
		}

		auto pSet = offsetMemory(srcObj, offset, isArrayRef);
		srcPtsSets.emplace_back(pSet);
	}

	PtsSet resSet = PtsSet::mergeAll(srcPtsSets);

	// For now let's assume that if srcSet contains only null loc, the result should be a universal loc
	if (resSet.empty())
		resSet = PtsSet::getSingletonSet(MemoryManager::getUniversalObject());

	return env.strongUpdate(dst, resSet);
}

void TransferFunction::evalOffsetNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto const& offsetNode = static_cast<const OffsetCFGNode&>(*pp.getCFGNode());
	auto& ptrManager = globalState.getPointerManager();

	auto srcPtr = ptrManager.getPointer(ctx, offsetNode.getSrc());
	if (srcPtr == nullptr)
		return;
	auto dstPtr = ptrManager.getOrCreatePointer(ctx, offsetNode.getDest());

	auto envChanged = copyWithOffset(dstPtr, srcPtr, offsetNode.getOffset(), offsetNode.isArrayRef());

	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
}

}
