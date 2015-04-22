#include "Client/Taintness/Precision/LocalDataFlowTracker.h"

#include <llvm/IR/Instruction.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

LocalDataFlowTracker::LocalDataFlowTracker(LocalWorkList& l): localWorkList(l)
{
}

void LocalDataFlowTracker::trackValues(const DefUseInstruction* duInst, const ValueSet& values)
{
	assert(!duInst->isEntryInstruction());

	// Enqueue all affected operands
	for (auto predDuInst: duInst->top_preds())
	{
		if (predDuInst->isEntryInstruction() || values.count(predDuInst->getInstruction()))
			localWorkList.enqueue(predDuInst);
	}
}

void LocalDataFlowTracker::trackMemory(const DefUseInstruction* duInst, const MemorySet& locs)
{
	assert(!duInst->isEntryInstruction());

	// Enqueue all affected operands
	for (auto const& mapping: duInst->mem_preds())
	{
		if (!locs.count(mapping.first))
			continue;
		
		for (auto predDuInst: mapping.second)
			localWorkList.enqueue(predDuInst);
	}
}

}
}