#ifndef TPA_TUNABLE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_H

#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "TPA/DataFlow/Memo.h"

namespace tpa
{

class PointerCFGNode;
class PointerProgram;

class TunablePointerAnalysis: public PointerAnalysis
{
private:
	Memo<PointerCFGNode> memo;
public:
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerEffectTable& e);
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerEffectTable& e, const Env&);
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerEffectTable& e, Env&&);
	~TunablePointerAnalysis();

	void runOnProgram(const PointerProgram& prog, Store store);
};

}

#endif
