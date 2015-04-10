#ifndef TPA_TAINT_SINK_VIOLATION_CLASSIFIER_H
#define TPA_TAINT_SINK_VIOLATION_CLASSIFIER_H

#include "Client/Taintness/SourceSink/SinkViolationChecker.h"
#include "Utils/Hashing.h"

#include <unordered_map>

namespace tpa
{
	class DefUseFunction;
	class DefUseInstruction;
	class DefUseModule;
}

namespace client
{
namespace taint
{

class ContextDefUseFunction
{
private:
	const tpa::Context* ctx;
	const tpa::DefUseFunction* duFunc;

	using PairType = std::pair<decltype(ctx), decltype(duFunc)>;
public:
	ContextDefUseFunction(const tpa::Context* c, const tpa::DefUseFunction* f): ctx(c), duFunc(f)
	{
	}

	const tpa::Context* getContext() const { return ctx; }
	const tpa::DefUseFunction* getDefUseFunction() const { return duFunc; }

	bool operator==(const ContextDefUseFunction& rhs) const
	{
		return ctx == rhs.ctx && duFunc == rhs.duFunc;
	}
	bool operator!=(const ContextDefUseFunction& rhs) const
	{
		return !(*this == rhs);
	}

	operator PairType() const
	{
		return std::make_pair(ctx, duFunc);
	}
};

class FunctionSinkViolationRecord
{
private:
	const tpa::DefUseInstruction* duInst;
	SinkViolationRecords records;
public:
	FunctionSinkViolationRecord(const tpa::DefUseInstruction* i, const SinkViolationRecords& r): duInst(i), records(r) {}
	//FunctionSinkViolationRecord(const tpa::DefUseInstruction* i, SinkViolationRecords&& r): duInst(i), records(std::move(r)) {}

	const tpa::DefUseInstruction* getDefUseInstruction() const
	{
		return duInst;
	}

	const SinkViolationRecords& getSinkViolationRecords() const
	{
		return records;
	}
};
using FunctionSinkViolationRecords = std::vector<FunctionSinkViolationRecord>;

// Classify sink violation records based on what function the violation is in
class SinkViolationClassifier
{
private:
	const tpa::DefUseModule& duModule;

	std::unordered_map<ContextDefUseFunction, FunctionSinkViolationRecords, tpa::PairHasher<const tpa::Context*, const tpa::DefUseFunction*>> recordsMap;
public:
	using const_iterator = decltype(recordsMap)::const_iterator;

	SinkViolationClassifier(const tpa::DefUseModule& m);

	void addSinkSignature(const tpa::ProgramLocation&, const SinkViolationRecords&);

	const_iterator begin() const { return recordsMap.begin(); }
	const_iterator end() const { return recordsMap.end(); }
};

}
}

#endif
