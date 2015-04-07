#ifndef TPA_TAINT_ANALYSIS_ENGINE_H
#define TPA_TAINT_ANALYSIS_ENGINE_H

#include "Client/Taintness/DataFlow/TaintTransferFunction.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "TPA/DataFlow/TPAWorkList.h"

namespace client
{
namespace taint
{

class TaintGlobalState;

class TaintAnalysisEngine
{
private:
	TaintGlobalState& globalState;

	tpa::TPAWorkList<tpa::DefUseFunction> globalWorkList;

	void initializeWorkList();
	void evalFunction(const tpa::Context*, const tpa::DefUseFunction*);

	// initializeWorkList helpers
	void initializeMainArgs(TaintEnv& env, TaintStore& store, const tpa::DefUseFunction& entryDuFunc);
	void initializeExternalGlobalValues(TaintEnv& env, TaintStore& store, const tpa::DefUseModule& duModule);
	void enqueueEntryPoint(const tpa::DefUseFunction& entryDuFunc, TaintStore store);
public:
	TaintAnalysisEngine(TaintGlobalState& g);

	bool run();
};

}
}

#endif
