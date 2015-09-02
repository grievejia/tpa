#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"
#include "PointerAnalysis/Program/CFG/CFG.h"

namespace tpa
{

void TransferFunction::evalEntryNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	assert(localState != nullptr);

	addTopLevelSuccessors(pp, evalResult);
	addMemLevelSuccessors(pp, *localState, evalResult);

	// To prevent the analysis to converge before a newly added return edge is processed, we need to force the analysis to check the return targets whenever a function is entered
	auto& cfg = pp.getCFGNode()->getCFG();
	if (!cfg.doesNotReturn())
		evalResult.addTopLevelProgramPoint(ProgramPoint(pp.getContext(), cfg.getExitNode()));
}

}
