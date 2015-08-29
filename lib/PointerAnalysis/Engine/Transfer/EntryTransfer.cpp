#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/TransferFunction.h"

namespace tpa
{

void TransferFunction::evalEntryNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	assert(localState != nullptr);

	//auto prunedStore = globalState.getStorePruner().pruneStore(*localState, pp);
	//auto const& store = evalResult.getNewStore(std::move(prunedStore));

	addTopLevelSuccessors(pp, evalResult);
	addMemLevelSuccessors(pp, *localState, evalResult);
}

}
