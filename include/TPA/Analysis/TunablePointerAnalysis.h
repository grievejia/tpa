#ifndef TPA_TUNABLE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_H

#include "MemoryModel/PtsSet/Store.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

namespace tpa
{

class PointerCFGNode;
class PointerProgram;

class TunablePointerAnalysis: public PointerAnalysis
{
public:
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerTable& e);
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerTable& e, const Env&);
	TunablePointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerTable& e, Env&&);
	~TunablePointerAnalysis();

	void runOnProgram(const PointerProgram& prog, Store store);
};

}

#endif
