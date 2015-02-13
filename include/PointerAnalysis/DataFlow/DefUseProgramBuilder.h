#ifndef TPA_DEF_USE_GRAPH_BUILDER_H
#define TPA_DEF_USE_GRAPH_BUILDER_H

#include "PointerAnalysis/DataFlow/DefUseProgram.h"

namespace tpa
{

class ExternalPointerEffectTable;
class ModRefSummaryMap;
class PointerAnalysis;
class PointerProgram;

class DefUseProgramBuilder
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalPointerEffectTable& extTable;

	void buildDefUseGraph(DefUseGraph& dug, const PointerCFG& cfg);
public:
	DefUseProgramBuilder(const PointerAnalysis& p, const ModRefSummaryMap& m, const ExternalPointerEffectTable& t): ptrAnalysis(p), summaryMap(m), extTable(t) {}
	DefUseProgram buildDefUseProgram(const PointerProgram& prog);
};

}

#endif
