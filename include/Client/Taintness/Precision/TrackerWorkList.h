#ifndef TPA_TAINT_TRACKER_WORKLIST_H
#define TPA_TAINT_TRACKER_WORKLIST_H

#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "Utils/WorkList.h"

namespace client
{
namespace taint
{

struct DefUsePrioComparator
{
	bool operator()(const tpa::DefUseInstruction* i0, const tpa::DefUseInstruction* i1) const
	{
		return i0->getPriority() > i1->getPriority();
	}
};

using TrackerWorkList = tpa::PrioWorkList<const tpa::DefUseInstruction*, DefUsePrioComparator>;

}
}

#endif
