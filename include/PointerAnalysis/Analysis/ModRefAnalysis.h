#ifndef TPA_MOD_REF_ANALYSIS_H
#define TPA_MOD_REF_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ModRefSummary.h"

namespace tpa
{

class ExternalPointerEffectTable;
class PointerAnalysis;
class PointerCFG;
class PointerProgram;

// A context-insensitive mod-ref analysis backed up by a pointer analysis
class ModRefAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ExternalPointerEffectTable& extTable;

	void collectProcedureSummary(const PointerCFG&, ModRefSummary&);
public:
	ModRefAnalysis(const PointerAnalysis& p, const ExternalPointerEffectTable& t): ptrAnalysis(p), extTable(t) {}

	ModRefSummaryMap runOnProgram(const PointerProgram& prog);
};

}

#endif
