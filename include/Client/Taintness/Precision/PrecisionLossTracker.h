#ifndef TPA_TAINT_PRECISION_LOSS_TRACKER_H
#define TPA_TAINT_PRECISION_LOSS_TRACKER_H

#include "Client/Taintness/SourceSink/Checker/SinkViolationRecord.h"
#include "Client/Taintness/Precision/PrecisionLossGlobalState.h"
#include "Client/Taintness/Precision/TrackerTypes.h"
#include "PointerAnalysis/DataFlow/DefUseProgramLocation.h"

namespace client
{
namespace taint
{

class ContextDefUseFunction;
class TaintGlobalState;

class PrecisionLossTracker
{
private:
	const TaintGlobalState& globalState;
	GlobalWorkList globalWorkList;
public:
	PrecisionLossTracker(const TaintGlobalState&);

	void addSinkViolation(const tpa::DefUseProgramLocation&, const SinkViolationRecords&);

	DefUseProgramLocationSet trackImprecisionSource();
};

}
}

#endif
