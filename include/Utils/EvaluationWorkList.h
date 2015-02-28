#ifndef TPA_EVALUATION_WORKLIST_H
#define TPA_EVALUATION_WORKLIST_H

#include "Utils/Hashing.h"
#include "Utils/WorkList.h"

#include <unordered_map>

namespace tpa
{

class Context;

template <typename ElemListType, typename ElemType, typename ElemPrioComparator = std::less<ElemType>>
class EvaluationWorkList
{
private:
	// The function-level worklist
	using ContextFunctionPair = std::pair<const Context*, const ElemListType*>;
	using FunctionWorkList = WorkList<ContextFunctionPair, PairHasher<const Context*, const ElemListType*>>;
public:
	// The intra-procedural level worklist
	using LocalWorkList = PrioWorkList<const ElemType*, ElemPrioComparator>;
private:
	using WorkListMap = std::unordered_map<ContextFunctionPair, LocalWorkList, PairHasher<const Context*, const ElemListType*>>;

	FunctionWorkList funWorkList;
	WorkListMap workListMap;

	ElemPrioComparator comp;
public:
	EvaluationWorkList(const ElemPrioComparator& c = ElemPrioComparator()): comp(c) {}

	bool isEmpty() const
	{
		return funWorkList.isEmpty();
	}

	void enqueue(const Context* ctx, const ElemListType* g)
	{
		funWorkList.enqueue(std::make_pair(ctx, g));
	}

	void enqueue(const Context* ctx, const ElemListType* g, const ElemType* node)
	{
		auto pair = std::make_pair(ctx, g);
		funWorkList.enqueue(pair);

		auto itr = workListMap.find(pair);
		if (itr == workListMap.end())
			itr = workListMap.insert(std::make_pair(pair, LocalWorkList(comp))).first;
		itr->second.enqueue(node);
	}

	ContextFunctionPair dequeue()
	{
		assert(!funWorkList.isEmpty());
		return funWorkList.dequeue();
	}

	LocalWorkList& getLocalWorkList(const Context* ctx, const ElemListType* g)
	{
		auto itr = workListMap.find(std::make_pair(ctx, g));
		assert(itr != workListMap.end());
		// The dequeued worklist may be empty if recursive call is involved
		//assert(!itr->second.isEmpty());
		return itr->second;
	}
};

}

#endif
