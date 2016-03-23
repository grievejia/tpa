#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"

namespace tpa
{

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

void TransferFunction::evalStore(const Pointer* dst, const Pointer* src, const ProgramPoint& pp, EvalResult& evalResult)
{
	auto& env = globalState.getEnv();

	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return;

	auto dstSet = env.lookup(dst);
	if (dstSet.empty())
		return;

	auto& store = evalResult.getNewStore(*localState);

	auto dstObj = *dstSet.begin();
	// If the store target is precise and the target location is not unknown
	if (dstSet.size() == 1 && !dstObj->isSummaryObject())
		strongUpdateStore(dstObj, srcSet, store);
	else
		weakUpdateStore(dstSet, srcSet, store);

	addMemLevelSuccessors(pp, store, evalResult);
}

void TransferFunction::evalStoreNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto const& storeNode = static_cast<const StoreCFGNode&>(*pp.getCFGNode());

	auto& ptrManager = globalState.getPointerManager();
	auto srcPtr = ptrManager.getPointer(ctx, storeNode.getSrc());
	auto dstPtr = ptrManager.getPointer(ctx, storeNode.getDest());

	if (srcPtr == nullptr || dstPtr == nullptr)
		return;

	evalStore(dstPtr, srcPtr, pp, evalResult);
}

}
