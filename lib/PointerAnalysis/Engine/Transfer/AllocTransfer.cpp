#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"

namespace tpa
{

bool TransferFunction::evalMemoryAllocation(const context::Context* ctx, const llvm::Instruction* inst, const TypeLayout* type, bool isHeap)
{
	auto ptr = globalState.getPointerManager().getOrCreatePointer(ctx, inst);

	auto mem = 
		isHeap?
		globalState.getMemoryManager().allocateHeapMemory(ctx, inst, type) :
		globalState.getMemoryManager().allocateStackMemory(ctx, inst, type);

	return globalState.getEnv().strongUpdate(ptr, PtsSet::getSingletonSet(mem));
}

void TransferFunction::evalAllocNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto allocNode = static_cast<const AllocCFGNode*>(pp.getCFGNode());
	auto envChanged = evalMemoryAllocation(pp.getContext(), allocNode->getDest(), allocNode->getAllocTypeLayout(), false);

	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
}

}
