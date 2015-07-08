#pragma once

#include "TaintAnalysis/Support/ProgramPointSet.h"
#include "TaintAnalysis/Support/SinkViolationRecord.h"

#include <unordered_set>

namespace taint
{

class TaintGlobalState;
class TrackerWorkList;

class PrecisionLossTracker
{
private:
	const TaintGlobalState& globalState;

	void initializeWorkList(TrackerWorkList&, const SinkViolationRecord&);
public:
	PrecisionLossTracker(const TaintGlobalState& g): globalState(g) {}

	ProgramPointSet trackImprecision(const SinkViolationRecord&);
};

}
