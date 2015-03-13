#ifndef TPA_TUNABLE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_H

#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "TPA/DataFlow/Memo.h"

namespace tpa
{

class PointerCFGNode;
class PointerProgram;
class Store;

class TunablePointerAnalysis: public PointerAnalysis
{
private:
	StoreManager& storeManager;

	Memo<PointerCFGNode> memo;
public:
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e);
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, const Env&);
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& ss, const ExternalPointerEffectTable& e, Env&&);
	~TunablePointerAnalysis();

	void runOnProgram(const PointerProgram& prog, Store store);
};

}

#endif
