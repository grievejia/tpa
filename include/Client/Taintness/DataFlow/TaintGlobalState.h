#ifndef TPA_TAINT_GLOBAL_STATE_H
#define TPA_TAINT_GLOBAL_STATE_H

#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "Client/Taintness/DataFlow/TaintMemo.h"
#include "Client/Taintness/SourceSink/SinkSignature.h"
#include "MemoryModel/Precision/ProgramLocation.h"

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

class SourceSinkLookupTable;

class TaintGlobalState
{
private:
	// The program
	const tpa::DefUseModule& duModule;

	// Pointer Analysis
	const tpa::PointerAnalysis& ptrAnalysis;

	// External table
	const tpa::ExternalPointerEffectTable& extTable;

	// SourceSink manager
	const SourceSinkLookupTable& sourceSinkLookupTable;

	// The environment
	TaintEnv env;

	// The global memo
	TaintMemo memo;

	using SinkSet = std::unordered_set<SinkSignature>;
	SinkSet sinkSet;

	std::unordered_set<tpa::ProgramLocation> visitedFuncs;
public:
	TaintGlobalState(const tpa::DefUseModule& m, const tpa::PointerAnalysis& p, const tpa::ExternalPointerEffectTable& e, const SourceSinkLookupTable& s): duModule(m), ptrAnalysis(p), extTable(e), sourceSinkLookupTable(s) {}

	const tpa::DefUseModule& getProgram() const { return duModule; }

	const tpa::PointerAnalysis& getPointerAnalysis() const { return ptrAnalysis; }
	const tpa::ExternalPointerEffectTable& getExternalPointerEffectTable() const { return extTable; }
	const SourceSinkLookupTable&getSourceSinkLookupTable() const { return sourceSinkLookupTable; }

	TaintEnv& getEnv() { return env; }
	const TaintEnv& getEnv() const { return env; }

	TaintMemo& getMemo() { return memo; }
	const TaintMemo& getMemo() const { return memo; }

	bool insertSink(const tpa::ProgramLocation& pLoc, const llvm::Function* f)
	{
		return sinkSet.insert(SinkSignature(pLoc, f)).second;
	}

	const SinkSet& getSinkSignatures() const
	{
		return sinkSet;
	}

	bool insertVisitedFunction(const tpa::ProgramLocation& pLoc)
	{
		return visitedFuncs.insert(pLoc).second;
	}
};

}
}

#endif
