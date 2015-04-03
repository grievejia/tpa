#ifndef TPA_POINTER_ANALYSIS_ENGINE_H
#define TPA_POINTER_ANALYSIS_ENGINE_H

#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "TPA/DataFlow/GlobalState.h"
#include "TPA/DataFlow/TPAWorkList.h"
#include "TPA/DataFlow/TransferFunction.h"

namespace tpa
{

class ExternalPointerEffectTable;
class MemoryManager;
class PointerManager;

class PointerAnalysisEngine
{
private:
	GlobalState<PointerProgram>& globalState;

	// TransferFunction knows how to update from one state to another
	TransferFunction<PointerCFG> transferFunction;

	// The worklist of this engine
	TPAWorkList<PointerCFG> globalWorkList;
	
	void initializeWorkList(Store store);
	void evalFunction(const Context*, const PointerCFG*);
public:
	PointerAnalysisEngine(PointerManager& p, MemoryManager& m, GlobalState<PointerProgram>& g, Store st, const ExternalPointerEffectTable& t);

	void run();
};

}

#endif
