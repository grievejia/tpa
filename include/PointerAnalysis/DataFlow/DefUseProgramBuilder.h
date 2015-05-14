#ifndef TPA_DEF_USE_GRAPH_BUILDER_H
#define TPA_DEF_USE_GRAPH_BUILDER_H

#include "PointerAnalysis/DataFlow/DefUseProgram.h"

namespace tpa
{

class ExternalModRefTable;
class ModRefSummaryMap;
class PointerAnalysis;
class PointerProgram;

class DefUseProgramBuilder
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModRefTable& modRefTable;

	void buildDefUseGraph(DefUseGraph& dug, const PointerCFG& cfg);
public:
	DefUseProgramBuilder(const PointerAnalysis& p, const ModRefSummaryMap& m, const ExternalModRefTable& t): ptrAnalysis(p), summaryMap(m), modRefTable(t) {}
	DefUseProgram buildDefUseProgram(const PointerProgram& prog);
};

}

#endif
