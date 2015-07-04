#pragma once

#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "PointerAnalysis/Support/FunctionContext.h"
#include "PointerAnalysis/Support/ProgramPoint.h"
#include "Util/DataStructure/FIFOWorkList.h"
#include "Util/DataStructure/PriorityWorkList.h"
#include "Util/DataStructure/TwoLevelWorkList.h"

namespace tpa
{

class WorkList
{
private:
	struct CFGNodeComparator
	{
		bool operator()(const CFGNode* lhs, const CFGNode* rhs) const
		{
			return lhs->getPriority() < rhs->getPriority();
		}
	};

	using GlobalWorkListType = util::FIFOWorkList<FunctionContext>;
	using LocalWorkListType = util::PriorityWorkList<const CFGNode*, CFGNodeComparator>;
	using WorkListType = util::TwoLevelWorkList<GlobalWorkListType, LocalWorkListType>;
	WorkListType workList;
public:
	using ElemType = ProgramPoint;

	WorkList() = default;

	void enqueue(const ProgramPoint& p)
	{
		workList.enqueue(std::make_pair(FunctionContext(p.getContext(), &p.getCFGNode()->getFunction()), p.getCFGNode()));
	}

	ProgramPoint dequeue()
	{
		auto pair = workList.dequeue();
		return ProgramPoint(pair.first.getContext(), pair.second);
	}

	ProgramPoint front()
	{
		auto pair = workList.front();
		return ProgramPoint(pair.first.getContext(), pair.second);
	}

	bool empty() const { return workList.empty(); }
};

}
