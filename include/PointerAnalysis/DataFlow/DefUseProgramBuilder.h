#ifndef TPA_DEF_USE_GRAPH_BUILDER_H
#define TPA_DEF_USE_GRAPH_BUILDER_H

#include "PointerAnalysis/DataFlow/DefUseProgram.h"

namespace tpa
{

class ExternalModTable;
class ExternalRefTable;
class ModRefSummaryMap;
class PointerAnalysis;
class PointerProgram;

class DefUseProgramBuilder
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModTable& extModTable;
	const ExternalRefTable& extRefTable;

	void buildDefUseGraph(DefUseGraph& dug, const PointerCFG& cfg);
public:
	DefUseProgramBuilder(const PointerAnalysis& p, const ModRefSummaryMap& m, const ExternalModTable& em, const ExternalRefTable& er): ptrAnalysis(p), summaryMap(m), extModTable(em), extRefTable(er) {}
	DefUseProgram buildDefUseProgram(const PointerProgram& prog);
};

}

#endif
