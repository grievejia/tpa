#ifndef TPA_TAINT_SINK_VIOLATION_CHECKER_H
#define TPA_TAINT_SINK_VIOLATION_CHECKER_H

#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "Client/Taintness/SourceSink/TaintDescriptor.h"

namespace tpa
{
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class SinkSignature;
class SourceSinkLookupTable;
class TSummary;

struct SinkViolationRecord
{
	unsigned argPos;
	TClass what;
	TaintLattice expectVal;
	TaintLattice actualVal;
};

class SinkViolationChecker
{
private:
	using SinkViolationRecords = std::vector<SinkViolationRecord>;

	const TaintEnv& env;
	const TaintStore& store;
	const SourceSinkLookupTable& table;
	const tpa::PointerAnalysis& ptrAnalysis;

	SinkViolationRecords checkCallSiteWithSummary(const tpa::ProgramLocation&, const TSummary&);
	TaintLattice lookupTaint(const tpa::ProgramLocation&, TClass);
public:
	SinkViolationChecker(const TaintEnv& e, const TaintStore& s, const SourceSinkLookupTable& t, const tpa::PointerAnalysis&);

	// Return all prog location whose sink policy is violated
	SinkViolationRecords checkSinkViolation(const SinkSignature&);
};

}
}

#endif
