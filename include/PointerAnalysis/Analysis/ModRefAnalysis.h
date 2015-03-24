#ifndef TPA_MOD_REF_ANALYSIS_H
#define TPA_MOD_REF_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ModRefSummary.h"

namespace tpa
{

class ExternalModTable;
class ExternalRefTable;
class PointerAnalysis;
class PointerCFG;
class PointerProgram;

// A context-insensitive mod-ref analysis backed up by a pointer analysis
class ModRefAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ExternalModTable& extModTable;
	const ExternalRefTable& extRefTable;

	void collectProcedureSummary(const PointerCFG&, ModRefSummary&);
public:
	ModRefAnalysis(const PointerAnalysis& p, const ExternalModTable& em, const ExternalRefTable& er): ptrAnalysis(p), extModTable(em), extRefTable(er) {}

	ModRefSummaryMap runOnProgram(const PointerProgram& prog);
};

}

#endif
