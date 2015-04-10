#ifndef TPA_TAINT_ANALYSIS_H
#define TPA_TAINT_ANALYSIS_H

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

class TaintAnalysis
{
private:
	SourceSinkLookupTable sourceSinkLookupTable;
	const tpa::PointerAnalysis& ptrAnalysis;
	const tpa::ExternalPointerEffectTable& extTable;

	bool checkSinkViolation(const TaintGlobalState& globalState);
public:
	TaintAnalysis(const tpa::PointerAnalysis& p, const tpa::ExternalPointerEffectTable& t);

	// Return true if there is a info flow violation
	bool runOnDefUseModule(const tpa::DefUseModule& m);
};

}
}

#endif
