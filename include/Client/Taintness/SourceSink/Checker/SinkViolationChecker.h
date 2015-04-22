#ifndef TPA_TAINT_SINK_VIOLATION_CHECKER_H
#define TPA_TAINT_SINK_VIOLATION_CHECKER_H

#include "Client/Taintness/DataFlow/TaintStore.h"
#include "Client/Taintness/SourceSink/Checker/SinkViolationRecord.h"

namespace tpa
{
	class PointerAnalysis;
	class ProgramLocation;
}

namespace client
{
namespace taint
{

class SinkSignature;
class SinkTaintEntry;
class SourceSinkLookupTable;
class TaintEnv;
class TaintSummary;

class SinkViolationChecker
{
private:
	const TaintEnv& env;
	const TaintStore& store;
	const SourceSinkLookupTable& table;
	const tpa::PointerAnalysis& ptrAnalysis;

	SinkViolationRecords checkCallSiteWithSummary(const tpa::DefUseProgramLocation&, const TaintSummary&);
	void checkCallSiteWithEntry(const tpa::DefUseProgramLocation&, const SinkTaintEntry&, SinkViolationRecords&);
	void checkValueWithTClass(const tpa::ProgramLocation&, TClass, uint8_t, SinkViolationRecords&);
	TaintLattice lookupTaint(const tpa::ProgramLocation&, TClass);
public:
	SinkViolationChecker(const TaintEnv& e, const TaintStore& s, const SourceSinkLookupTable& t, const tpa::PointerAnalysis&);

	// Return all prog location whose sink policy is violated
	SinkViolationRecords checkSinkViolation(const SinkSignature&);
};

}
}

#endif
