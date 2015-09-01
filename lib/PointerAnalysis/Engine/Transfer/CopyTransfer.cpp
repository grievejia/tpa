#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"

namespace tpa
{

void TransferFunction::evalCopyNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto const& copyNode = static_cast<const CopyCFGNode&>(*pp.getCFGNode());

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
			return;

		srcPtsSets.emplace_back(pSet);
	}

	auto dstPtr = ptrManager.getOrCreatePointer(ctx, copyNode.getDest());
	auto dstSet = PtsSet::mergeAll(srcPtsSets);
	auto envChanged = globalState.getEnv().strongUpdate(dstPtr, dstSet);
	
	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
}

}
