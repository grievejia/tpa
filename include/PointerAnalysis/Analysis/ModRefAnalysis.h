#ifndef TPA_MOD_REF_ANALYSIS_H
#define TPA_MOD_REF_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ModRefSummary.h"

namespace tpa
{

class ExternalModRefTable;
class PointerAnalysis;
class PointerCFG;
class PointerProgram;

// A context-insensitive mod-ref analysis backed up by a pointer analysis
class ModRefAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ExternalModRefTable& modRefTable;

	void collectProcedureSummary(const PointerCFG&, ModRefSummary&);
public:
	ModRefAnalysis(const PointerAnalysis& p, const ExternalModRefTable& t): ptrAnalysis(p), modRefTable(t) {}

	ModRefSummaryMap runOnProgram(const PointerProgram& prog);
};

}

#endif
