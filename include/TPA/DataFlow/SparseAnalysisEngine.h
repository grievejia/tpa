#ifndef TPA_SPARSE_ANALYSIS_ENGINE_H
#define TPA_SPARSE_ANALYSIS_ENGINE_H

#include "PointerAnalysis/DataFlow/DefUseGraph.h"
#include "TPA/DataFlow/TPAWorkList.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/TransferFunction.h"

namespace tpa
{

class DefUseProgram;
class Env;
class ExternalPointerEffectTable;
class MemoryManager;
class ModRefSummaryMap;
class PointerManager;
class StaticCallGraph;
class StoreManager;

class SparseAnalysisEngine
{
private:
	// Call graph
	StaticCallGraph& callGraph;

	// The global memo
	Memo<DefUseGraphNode>& memo;

	// The store manager
	StoreManager& storeManager;

	// TransferFunction knows how to update from one state to another
	TransferFunction<DefUseGraph> transferFunction;

	// Modref summary
	const ModRefSummaryMap& summaryMap;

	using GlobalWorkList = TPAWorkList<DefUseGraph>;
	using LocalWorkList = GlobalWorkList::LocalWorkList;

	void evalFunction(const Context*, const DefUseGraph*, Env&, GlobalWorkList&, const DefUseProgram&);
	void applyFunction(const Context*, const CallDefUseNode*, const llvm::Function*, const DefUseProgram& prog, Env&, Store, GlobalWorkList&, LocalWorkList&);
	Store pruneStore(const Store& store, const llvm::Function* f);
	void propagateTopLevel(const DefUseGraphNode* node, LocalWorkList& workList);
	void propagateMemoryLevel(const Context*, const DefUseGraphNode*, const Store&, LocalWorkList&);
public:
	SparseAnalysisEngine(PointerManager& p, MemoryManager& m, StoreManager& s, StaticCallGraph& g, Memo<DefUseGraphNode>& me, const ModRefSummaryMap& sm, const ExternalPointerEffectTable& t): callGraph(g), memo(me), storeManager(s), transferFunction(p, m, s, t), summaryMap(sm) {}

	void runOnDefUseProgram(const DefUseProgram&, Env&, Store);
};

}

#endif
