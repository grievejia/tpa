#pragma once

#include "PointerAnalysis/Support/FunctionContext.h"
#include "PointerAnalysis/Support/ProgramPoint.h"
#include "Util/DataStructure/VectorSet.h"
#include "Util/Iterator/IteratorRange.h"

#include <unordered_map>

namespace tpa
{

class ProgramPoint;

class CallGraph
{
private:
	using CalleeSet = util::VectorSet<FunctionContext>;
	using CallerSet = util::VectorSet<ProgramPoint>;
	using CalleeConstIterator = typename CalleeSet::const_iterator;
	using CallerConstIterator = typename CallerSet::const_iterator;

	using CalleeMap = std::unordered_map<ProgramPoint, CalleeSet>;
	using CallerMap = std::unordered_map<FunctionContext, CallerSet>;

	CalleeMap calleeMap;
	CallerMap callerMap;
public:
	CallGraph() = default;

	bool insertEdge(const ProgramPoint&, const FunctionContext&);

	util::IteratorRange<CalleeConstIterator> getCallees(const ProgramPoint&) const;
	util::IteratorRange<CallerConstIterator> getCallers(const FunctionContext&) const;
};

}
