#ifndef TPA_EVALUATION_WORKLIST_H
#define TPA_EVALUATION_WORKLIST_H

#include "ControlFlow/PointerCFG.h"
#include "Utils/STLUtils.h"
#include "Utils/WorkList.h"

#include <unordered_map>

namespace tpa
{

class Context;

class EvaluationWorkList
{
private:
	// The function-level worklist
	using ContextFunctionPair = std::pair<const Context*, const PointerCFG*>;
	using FunctionWorkList = WorkList<ContextFunctionPair, PairHasher<const Context*, const PointerCFG*>>;

	// The intra-procedural level worklist
	struct PrioCompare
	{
		bool operator()(const PointerCFGNode* n0, const PointerCFGNode* n1) const
		{
			return n0->getPriority() < n1->getPriority();
		}
	};
public:
	using LocalWorkList = PrioWorkList<const PointerCFGNode*, PrioCompare>;
private:
	using WorkListMap = std::unordered_map<ContextFunctionPair, LocalWorkList, PairHasher<const Context*, const PointerCFG*>>;

	FunctionWorkList funWorkList;
	WorkListMap workListMap;
public:
	EvaluationWorkList() = default;

	bool isEmpty() const;

	void enqueue(const Context*, const PointerCFG*, const PointerCFGNode*);
	ContextFunctionPair dequeue();

	LocalWorkList& getLocalWorkList(const Context*, const PointerCFG*);
};

}

#endif
