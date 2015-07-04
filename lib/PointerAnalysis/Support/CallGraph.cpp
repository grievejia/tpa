#include "Context/Context.h"
#include "PointerAnalysis/Program/CFG/CFG.h"
#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "PointerAnalysis/Support/CallGraph.h"
#include "PointerAnalysis/Support/ProgramPoint.h"

using namespace util;

namespace tpa
{

template <typename MapType, typename KeyType, typename ValueType>
static inline bool insertMap(MapType& m, const KeyType& k, const ValueType& v)
{
	using SetType = VectorSet<ValueType>;

	auto mapInsertPair = m.insert(std::make_pair(k, SetType()));
	auto setInsertPair = mapInsertPair.first->second.insert(v);

	return mapInsertPair.second || setInsertPair.second;
}

bool CallGraph::insertEdge(const ProgramPoint& pp, const FunctionContext& fc)
{
	auto ret0 = insertMap(calleeMap, pp, fc);
	auto ret1 = insertMap(callerMap, fc, pp);
	return ret0 || ret1;
}

IteratorRange<CallGraph::CalleeConstIterator> CallGraph::getCallees(const ProgramPoint& pp) const
{
	auto itr = calleeMap.find(pp);
	if (itr == calleeMap.end())
		return iteratorRange(CalleeConstIterator(), CalleeConstIterator());
	else
		return iteratorRange(itr->second.begin(), itr->second.end());
}

IteratorRange<CallGraph::CallerConstIterator> CallGraph::getCallers(const FunctionContext& fc) const
{
	auto itr = callerMap.find(fc);
	if (itr == callerMap.end())
		return iteratorRange(CallerConstIterator(), CallerConstIterator());
	else
		return iteratorRange(itr->second.begin(), itr->second.end());
}

}