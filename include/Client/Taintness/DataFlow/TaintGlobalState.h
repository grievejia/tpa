#ifndef TPA_TAINT_GLOBAL_STATE_H
#define TPA_TAINT_GLOBAL_STATE_H

#include "Client/Taintness/DataFlow/TaintEnv.h"
#include "Client/Taintness/DataFlow/TaintMemo.h"
#include "Client/Taintness/SourceSink/Checker/SinkSignature.h"
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

class TaintGlobalState
{
private:
	// The program
	const tpa::DefUseModule& duModule;

	// Pointer Analysis
	const tpa::PointerAnalysis& ptrAnalysis;

	// External taink annotations
	SourceSinkLookupTable sourceSinkLookupTable;

	// The environment
	TaintEnv env;

	// The global memo
	TaintMemo memo;

	using SinkSet = std::unordered_set<SinkSignature>;
	SinkSet sinkSet;

	std::unordered_set<tpa::ProgramLocation> visitedFuncs;
public:
	TaintGlobalState(const tpa::DefUseModule& m, const tpa::PointerAnalysis& p, const llvm::StringRef& fileName = "source_sink.conf"): duModule(m), ptrAnalysis(p)
	{
		sourceSinkLookupTable.readSummaryFromFile(fileName);
	}

	const tpa::DefUseModule& getProgram() const { return duModule; }

	const tpa::PointerAnalysis& getPointerAnalysis() const { return ptrAnalysis; }
	const SourceSinkLookupTable& getSourceSinkLookupTable() const { return sourceSinkLookupTable; }

	TaintEnv& getEnv() { return env; }
	const TaintEnv& getEnv() const { return env; }

	TaintMemo& getMemo() { return memo; }
	const TaintMemo& getMemo() const { return memo; }

	bool insertSink(const tpa::DefUseProgramLocation& pLoc, const llvm::Function* f)
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
