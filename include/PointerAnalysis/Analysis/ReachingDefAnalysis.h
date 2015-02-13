#ifndef TPA_REACHING_DEF_ANALYSIS_H
#define TPA_REACHING_DEF_ANALYSIS_H

#include "Utils/VectorSet.h"

#include <unordered_map>

namespace tpa
{

class ExternalPointerEffectTable;
class PointerAnalysis;
class PointerCFG;
class PointerCFGNode;
class MemoryLocation;
class ModRefSummaryMap;

class ReachingDefStore
{
private:
	using NodeSet = VectorSet<const PointerCFGNode*>;
	std::unordered_map<const MemoryLocation*, NodeSet> store;
public:
	using const_iterator = decltype(store)::const_iterator;

	NodeSet& getReachingDefs(const MemoryLocation* loc);
	bool insertBinding(const MemoryLocation* loc, const PointerCFGNode* node);
	bool mergeWith(const ReachingDefStore& rhs);
	// Return NULL if binding does not exist
	const NodeSet* getReachingDefs(const MemoryLocation* loc) const;

	const_iterator begin() const { return store.begin(); }
	const_iterator end() const { return store.end(); }
};

class ReachingDefMap
{
private:
	std::unordered_map<const PointerCFGNode*, ReachingDefStore> rdStore;
public:
	bool update(const PointerCFGNode* node, const ReachingDefStore& storeUpdate);
	ReachingDefStore& getReachingDefStore(const PointerCFGNode* node);
	const ReachingDefStore& getReachingDefStore(const PointerCFGNode* node) const;
};

class ReachingDefAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalPointerEffectTable& extTable;

	void evalNode(const PointerCFGNode* node, ReachingDefStore& store);
public:
	ReachingDefAnalysis(const PointerAnalysis& p, const ModRefSummaryMap& s, const ExternalPointerEffectTable& t): ptrAnalysis(p), summaryMap(s), extTable(t) {}

	ReachingDefMap runOnPointerCFG(const PointerCFG& cfg);
};

}

#endif
