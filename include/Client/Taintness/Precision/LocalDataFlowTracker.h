#ifndef TPA_TAINT_LOCAL_DATAFLOW_TRACKER_H
#define TPA_TAINT_LOCAL_DATAFLOW_TRACKER_H

#include "Client/Taintness/Precision/TrackerTypes.h"

namespace client
{
namespace taint
{

class LocalDataFlowTracker
{
private:
	LocalWorkList& localWorkList;
public:
	LocalDataFlowTracker(LocalWorkList&);

	void trackValues(const tpa::DefUseInstruction*, const ValueSet&);
	void trackMemory(const tpa::DefUseInstruction*, const MemorySet&);
};

}
}

#endif
