#ifndef TPA_REACHING_DEF_ANALYSIS_H
#define TPA_REACHING_DEF_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ReachingDefMap.h"

namespace tpa
{

class ExternalModRefTable;
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
	const ExternalModRefTable& modRefTable;

	void evalNode(const PointerCFGNode* node, ReachingDefStore<PointerCFGNode>& store);
public:
	ReachingDefAnalysis(const PointerAnalysis& p, const ModRefSummaryMap& s, const ExternalModRefTable& t): ptrAnalysis(p), summaryMap(s), modRefTable(t) {}

	ReachingDefMap<PointerCFGNode> runOnPointerCFG(const PointerCFG& cfg);
};

}

#endif
