#ifndef TPA_POINTER_ANALYSIS_ENGINE_H
#define TPA_POINTER_ANALYSIS_ENGINE_H

#include "TPA/DataFlow/SemiSparseGlobalState.h"
#include "TPA/DataFlow/TPAWorkList.h"

namespace tpa
{

class MemoryManager;
class PointerManager;

class PointerAnalysisEngine
{
private:
	SemiSparseGlobalState& globalState;

	// The worklist of this engine
	TPAWorkList<PointerCFG> globalWorkList;
	
	void initializeWorkList(Store store);
	void evalFunction(const Context*, const PointerCFG*);
public:
	PointerAnalysisEngine(SemiSparseGlobalState& g, Store st);

	void run();
};

}

#endif
