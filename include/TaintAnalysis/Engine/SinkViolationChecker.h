#pragma once

#include "Annotation/Taint/TaintDescriptor.h"
#include "TaintAnalysis/Support/SinkViolationRecord.h"
#include "TaintAnalysis/Support/TaintStore.h"

namespace annotation
{
	class ExternalTaintTable;
	class SinkTaintEntry;
	class TaintSummary;
}

namespace tpa
{
	class SemiSparsePointerAnalysis;
}

namespace taint
{

class SinkSignature;
class TaintEnv;
class TaintMemo;
class TaintValue;

class SinkViolationChecker
{
private:
	const TaintEnv& env;
	const TaintMemo& memo;
	const annotation::ExternalTaintTable& table;
	const tpa::SemiSparsePointerAnalysis& ptrAnalysis;

	TaintLattice lookupTaint(const TaintValue&, annotation::TClass, const TaintStore*);
	void checkValueWithTClass(const TaintValue&, annotation::TClass, uint8_t, const TaintStore*, SinkViolationList&);
	void checkCallSiteWithEntry(const ProgramPoint&, const annotation::SinkTaintEntry&, SinkViolationList&);
	SinkViolationList checkCallSiteWithSummary(const ProgramPoint&, const annotation::TaintSummary&);

	void checkSinkViolation(const SinkSignature&, SinkViolationRecord&);
public:
	SinkViolationChecker(const TaintEnv& e, const TaintMemo& m, const annotation::ExternalTaintTable& t, const tpa::SemiSparsePointerAnalysis& p): env(e), memo(m), table(t), ptrAnalysis(p) {}

	// Return all prog location whose sink policy is violated
	template <typename SigContainer>
	SinkViolationRecord checkSinkViolation(const SigContainer& sinks)
	{
		SinkViolationRecord ret;
		for (auto const& sig: sinks)
			checkSinkViolation(sig, ret);
		return ret;
	}
};

}
