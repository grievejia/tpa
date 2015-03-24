#ifndef TPA_REACHING_DEF_ANALYSIS_H
#define TPA_REACHING_DEF_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ReachingDefMap.h"

namespace tpa
{

class ExternalModTable;
class PointerAnalysis;
class PointerCFG;
class PointerCFGNode;
class MemoryLocation;
class ModRefSummaryMap;

class ReachingDefAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModTable& extModTable;

	void evalNode(const PointerCFGNode* node, ReachingDefStore<PointerCFGNode>& store);
public:
	ReachingDefAnalysis(const PointerAnalysis& p, const ModRefSummaryMap& s, const ExternalModTable& t): ptrAnalysis(p), summaryMap(s), extModTable(t) {}

	ReachingDefMap<PointerCFGNode> runOnPointerCFG(const PointerCFG& cfg);
};

}

#endif
