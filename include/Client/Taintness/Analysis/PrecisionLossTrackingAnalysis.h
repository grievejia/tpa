#ifndef TPA_TAINT_PRECISION_LOSS_TRACKING_ANALYSIS_H
#define TPA_TAINT_PRECISION_LOSS_TRACKING_ANALYSIS_H

#include "Client/Taintness/SourceSink/SourceSinkLookupTable.h"

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
	SourceSinkLookupTable sourceSinkLookupTable;
	const tpa::PointerAnalysis& ptrAnalysis;
	const tpa::ExternalPointerEffectTable& extTable;

	void checkSinkViolation(const TaintGlobalState& globalState);
public:
	PrecisionLossTrackingAnalysis(const tpa::PointerAnalysis& p, const tpa::ExternalPointerEffectTable& t);

	// Return true if there is a info flow violation
	void runOnDefUseModule(const tpa::DefUseModule& m);
};

}
}

#endif
