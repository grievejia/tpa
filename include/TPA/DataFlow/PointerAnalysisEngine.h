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

	// Call graph
	StaticCallGraph& callGraph;

	// The global memo
	Memo<PointerCFGNode>& memo;

	// TransferFunction knows how to update from one state to another
	TransferFunction<PointerCFG> transferFunction;

	using GlobalWorkList = TPAWorkList<PointerCFG>;
	using LocalWorkList = GlobalWorkList::LocalWorkList;
	
	void evalFunction(const Context*, const PointerCFG*, Env&, GlobalWorkList&, const PointerProgram&);
	void applyFunction(const Context*, const CallNode*, const llvm::Function*, const PointerProgram& prog, Env&, Store, GlobalWorkList&, LocalWorkList&);
	void propagateTopLevel(const PointerCFGNode* node, LocalWorkList& workList);
	void propagateMemoryLevel(const Context*, const PointerCFGNode*, const Store&, LocalWorkList&);
public:
	PointerAnalysisEngine(PointerManager& p, MemoryManager& m, StoreManager& s, StaticCallGraph& g, Memo<PointerCFGNode>& me, const ExternalPointerEffectTable& t): callGraph(g), memo(me), transferFunction(p, m, s, t) {}

	void runOnProgram(const PointerProgram&, Env&, Store);
};

}

#endif
