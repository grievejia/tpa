#ifndef TPA_TAINT_GLOBAL_STATE_H
#define TPA_TAINT_GLOBAL_STATE_H

#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "Client/Taintness/DataFlow/TaintMemo.h"
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

class SourceSinkManager;

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
	const SourceSinkManager& ssManager;

	// The environment
	TaintEnv env;

	// The global memo
	TaintMemo memo;

	std::unordered_set<tpa::ProgramLocation> visitedFuncs;
public:
	TaintGlobalState(const tpa::DefUseModule& m, const tpa::PointerAnalysis& p, const tpa::ExternalPointerEffectTable& e, const SourceSinkManager& s): duModule(m), ptrAnalysis(p), extTable(e), ssManager(s) {}

	const tpa::DefUseModule& getProgram() const { return duModule; }

	const tpa::PointerAnalysis& getPointerAnalysis() const { return ptrAnalysis; }
	const tpa::ExternalPointerEffectTable& getExternalPointerEffectTable() const { return extTable; }
	const SourceSinkManager& getSourceSinkManager() const { return ssManager; }

	TaintEnv& getEnv() { return env; }
	const TaintEnv& getEnv() const { return env; }

	TaintMemo& getMemo() { return memo; }
	const TaintMemo& getMemo() const { return memo; }

	bool insertVisitedFunction(const tpa::ProgramLocation& pLoc)
	{
		return visitedFuncs.insert(pLoc).second;
	}
};

}
}

#endif
