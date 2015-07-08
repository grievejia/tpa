#pragma once

#include "PointerAnalysis/Support/FunctionContext.h"
#include "TaintAnalysis/Precision/IndexVector.h"
#include "TaintAnalysis/Support/ProgramPoint.h"

namespace tpa
{
	class MemoryObject;
}

namespace taint
{

class ProgramPoint;
class TrackerGlobalState;
class TrackerWorkList;

class ReturnTracker
{
private:
	TrackerGlobalState& trackerState;
	TrackerWorkList& workList;

	using CalleeVector = std::vector<tpa::FunctionContext>;
	using ReturnVector = std::vector<ProgramPoint>;

	ReturnVector getReturnVector(const CalleeVector&);
	TaintVector getReturnTaintValues(const ReturnVector&);
	TaintVector getReturnTaintValues(const tpa::MemoryObject*, const ReturnVector&);
	void processReturns(const ProgramPoint&, const ReturnVector&, const TaintVector&);
	void propagateReturns(const ReturnVector&, const TaintVector&);

	void trackValue(const ProgramPoint&, const ReturnVector&);
	void trackMemory(const ProgramPoint&, const ReturnVector&);
public:
	ReturnTracker(TrackerGlobalState& g, TrackerWorkList& w): trackerState(g), workList(w) {}

	void trackReturn(const ProgramPoint&, const CalleeVector&);
};

}
