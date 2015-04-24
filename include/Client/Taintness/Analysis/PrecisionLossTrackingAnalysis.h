#ifndef TPA_TAINT_PRECISION_LOSS_TRACKING_ANALYSIS_H
#define TPA_TAINT_PRECISION_LOSS_TRACKING_ANALYSIS_H

#include "Client/Taintness/SourceSink/Table/SourceSinkLookupTable.h"

namespace tpa
{
	class DefUseModule;
	class ExternalPointerEffectTable;
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class TaintGlobalState;

class PrecisionLossTrackingAnalysis
{
private:
	const tpa::PointerAnalysis& ptrAnalysis;

	void checkSinkViolation(const TaintGlobalState& globalState);
public:
	PrecisionLossTrackingAnalysis(const tpa::PointerAnalysis& p);

	// Return true if there is a info flow violation
	void runOnDefUseModule(const tpa::DefUseModule& m);
};

}
}

#endif
