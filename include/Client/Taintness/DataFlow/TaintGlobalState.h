#ifndef TPA_TAINT_GLOBAL_STATE_H
#define TPA_TAINT_GLOBAL_STATE_H

#include "Client/Taintness/DataFlow/TaintEnv.h"
#include "Client/Taintness/DataFlow/TaintMemo.h"
#include "Client/Taintness/SourceSink/Checker/SinkSignature.h"
#include "Client/Taintness/SourceSink/Table/ExternalTaintTable.h"

namespace tpa
{
	class DefUseModule;
	class ExternalPointerTable;
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
	ExternalTaintTable extTaintTable;

	// The environment
	TaintEnv env;

	// The global memo
	TaintMemo memo;

	using SinkSet = std::unordered_set<SinkSignature>;
	SinkSet sinkSet;

	std::unordered_set<tpa::ProgramLocation> visitedFuncs;
public:
	TaintGlobalState(const tpa::DefUseModule& m, const tpa::PointerAnalysis& p): duModule(m), ptrAnalysis(p)
	{
		extTaintTable = ExternalTaintTable::loadFromFile();
	}

	const tpa::DefUseModule& getProgram() const { return duModule; }

	const tpa::PointerAnalysis& getPointerAnalysis() const { return ptrAnalysis; }
	const ExternalTaintTable& getExternalTaintTable() const { return extTaintTable; }

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
