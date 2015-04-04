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

	TaintTransferFunction transferFunction;

	tpa::TPAWorkList<tpa::DefUseFunction> globalWorkList;

	void initializeWorkList();
	void evalFunction(const tpa::Context*, const tpa::DefUseFunction*);
public:
	TaintAnalysisEngine(TaintGlobalState& g, const tpa::ExternalPointerEffectTable& t);

	bool run();
};

}
}

#endif
