#ifndef TPA_TPA_WORKLIST_H
#define TPA_TPA_WORKLIST_H

#include "Utils/EvaluationWorkList.h"

namespace tpa
{

template <typename GraphType>
struct TPAPrioComparator
{
	using NodeType = typename GraphType::NodeType;
	bool operator()(const NodeType* n0, const NodeType* n1) const
	{
		return n0->getPriority() < n1->getPriority();
	}
};

template <typename GraphType>
using TPAWorkList = EvaluationWorkList<GraphType, typename GraphType::NodeType, TPAPrioComparator<GraphType>>;

}

#endif
