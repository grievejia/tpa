#ifndef TPA_POINTER_ANALYSIS_ENGINE_H
#define TPA_POINTER_ANALYSIS_ENGINE_H

#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "TPA/DataFlow/TPAWorkList.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/TransferFunction.h"

namespace tpa
{

class Env;
class ExternalPointerEffectTable;
class MemoryManager;
class PointerManager;
class PointerProgram;
class StaticCallGraph;
class StoreManager;

class PointerAnalysisEngine
{
private:

	// The program
	const PointerProgram& prog;

	// Call graph
	StaticCallGraph& callGraph;

	// The environment
	Env& env;

	// The global memo
	Memo<PointerCFGNode>& memo;

	// TransferFunction knows how to update from one state to another
	TransferFunction<PointerCFG> transferFunction;

	// The worklist of this engine
	using GlobalWorkList = TPAWorkList<PointerCFG>;
	GlobalWorkList globalWorkList;
	using LocalWorkList = GlobalWorkList::LocalWorkList;
	
	void initializeWorkList(Store store);
	void evalFunction(const Context*, const PointerCFG*);
	void applyFunction(const Context*, const CallNode*, const llvm::Function*, Store, LocalWorkList&);
	void propagateTopLevel(const PointerCFGNode* node, LocalWorkList& workList);
	void propagateMemoryLevel(const Context*, const PointerCFGNode*, const Store&, LocalWorkList&);
public:
	PointerAnalysisEngine(PointerManager& p, MemoryManager& m, StoreManager& s, const PointerProgram& pp, Env& e, Store st, StaticCallGraph& g, Memo<PointerCFGNode>& me, const ExternalPointerEffectTable& t);

	void run();
};

}

#endif
