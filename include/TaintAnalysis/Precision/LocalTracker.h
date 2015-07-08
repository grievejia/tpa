#pragma once

#include "TaintAnalysis/Precision/TrackerWorkList.h"
#include "TaintAnalysis/Program/DefUseInstruction.h"
#include "TaintAnalysis/Support/ProgramPoint.h"

namespace taint
{

class LocalTracker
{
private:
	TrackerWorkList& workList;
public:
	LocalTracker(TrackerWorkList& w): workList(w) {}

	template <typename ValueSet>
	void trackValue(const ProgramPoint& pp, const ValueSet& vSet)
	{
		auto ctx = pp.getContext();
		auto duInst = pp.getDefUseInstruction();
		assert(!duInst->isEntryInstruction());

		// Enqueue all affected operands
		for (auto predDuInst: duInst->top_preds())
		{
			if (predDuInst->isEntryInstruction() || vSet.count(predDuInst->getInstruction()))
				workList.enqueue(ProgramPoint(ctx, predDuInst));
		}
	}

	template <typename MemSet>
	void trackMemory(const ProgramPoint& pp, const MemSet& mSet)
	{
		auto ctx = pp.getContext();
		auto duInst = pp.getDefUseInstruction();
		assert(!duInst->isEntryInstruction());

		// Enqueue all affected operands
		for (auto const& mapping: duInst->mem_preds())
		{
			if (!mSet.count(mapping.first))
				continue;
			
			for (auto predDuInst: mapping.second)
				workList.enqueue(ProgramPoint(ctx, predDuInst));
		}
	}
};

}