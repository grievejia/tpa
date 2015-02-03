#ifndef TPA_POINTER_ANALYSIS_ENGINE_H
#define TPA_POINTER_ANALYSIS_ENGINE_H

#include "TPA/DataFlow/TransferFunction.h"
#include "TPA/DataFlow/EvaluationWorkList.h"

namespace tpa
{

class CallNode;
class Env;
class ExternalPointerEffectTable;
class Memo;
class MemoryManager;
class PointerCFGNode;
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
	Memo& memo;

	// TransferFunction knows how to update from one state to another
	TransferFunction transferFunction;

	using LocalWorkList = EvaluationWorkList::LocalWorkList;
	
	void evalFunction(const Context*, const PointerCFG*, Env&, EvaluationWorkList&, const PointerProgram&);
	void applyFunction(const Context*, const CallNode*, const llvm::Function*, const PointerProgram& prog, Env&, Store, EvaluationWorkList&, LocalWorkList&);
	void propagateTopLevel(const PointerCFGNode* node, LocalWorkList& workList);
	void propagateMemoryLevel(const Context*, const PointerCFGNode*, const Store&, LocalWorkList&);
public:
	PointerAnalysisEngine(PointerManager& p, MemoryManager& m, StoreManager& s, StaticCallGraph& g, Memo& me, const ExternalPointerEffectTable& t): callGraph(g), memo(me), transferFunction(p, m, s, t) {}

	void runOnProgram(const PointerProgram&, Env&, Store);
};

}

#endif
