#include "DataFlow/EvaluationWorkList.h"

using namespace llvm;

namespace tpa
{

bool EvaluationWorkList::isEmpty() const
{
	return funWorkList.isEmpty();
}

void EvaluationWorkList::enqueue(const Context* ctx, const PointerCFG* cfg, const PointerCFGNode* node)
{
	auto pair = std::make_pair(ctx, cfg);
	funWorkList.enqueue(pair);
	workListMap[pair].enqueue(node);
}

EvaluationWorkList::ContextFunctionPair EvaluationWorkList::dequeue()
{
	assert(!funWorkList.isEmpty());
	return funWorkList.dequeue();
}

EvaluationWorkList::LocalWorkList& EvaluationWorkList::getLocalWorkList(const Context* ctx, const PointerCFG* cfg)
{
	auto itr = workListMap.find(std::make_pair(ctx, cfg));
	assert(itr != workListMap.end());
	// The dequeued worklist may be empty if recursive call is involved
	//assert(!itr->second.isEmpty());
	return itr->second;
}

}
