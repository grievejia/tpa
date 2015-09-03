#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"

namespace tpa
{

PtsSet TransferFunction::loadFromPointer(const Pointer* ptr, const Store& store)
{
	assert(ptr != nullptr);

	auto uObj = MemoryManager::getUniversalObject();
	auto srcSet = globalState.getEnv().lookup(ptr);
	if (!srcSet.empty())
	{
		std::vector<PtsSet> srcSets;
		srcSets.reserve(srcSet.size());
		
		for (auto obj: srcSet)
		{
			auto objSet = store.lookup(obj);
			if (!objSet.empty())
			{
				srcSets.emplace_back(objSet);
				if (objSet.has(uObj))
					break;
			}
		}

		return PtsSet::mergeAll(srcSets);
	}

	return PtsSet::getSingletonSet(uObj);
}

void TransferFunction::evalLoadNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto const& loadNode = static_cast<const LoadCFGNode&>(*pp.getCFGNode());

	auto& ptrManager = globalState.getPointerManager();
	auto srcPtr = ptrManager.getPointer(ctx, loadNode.getSrc());
	if (srcPtr == nullptr)
		return;

	//assert(srcPtr != nullptr && "LoadNode is evaluated before its src operand becomes available");
	auto dstPtr = ptrManager.getOrCreatePointer(ctx, loadNode.getDest());

	auto resSet = loadFromPointer(srcPtr, *localState);
	auto envChanged = globalState.getEnv().strongUpdate(dstPtr, resSet);
	
	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
	addMemLevelSuccessors(pp, *localState, evalResult);
}

}
