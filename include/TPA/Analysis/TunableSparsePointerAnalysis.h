#ifndef TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_H

#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "TPA/DataFlow/Memo.h"

namespace tpa
{

class DefUseGraphNode;
class DefUseProgram;
class MemoryManager;
class ModRefSummaryMap;
class StoreManager;

class TunableSparsePointerAnalysis: public PointerAnalysis
{
private:
	StoreManager& storeManager;
	const ModRefSummaryMap& summaryMap;

	Memo<DefUseGraphNode> memo;
public:
	TunableSparsePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const ModRefSummaryMap& sm);
	TunableSparsePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const ModRefSummaryMap& sm, const Env&);
	TunableSparsePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const ModRefSummaryMap& sm, Env&&);
	~TunableSparsePointerAnalysis();

	void runOnDefUseProgram(const DefUseProgram& prog, Store store);
};

}

#endif
