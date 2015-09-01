#include "PointerAnalysis/Engine/GlobalState.h"
#include "PointerAnalysis/Engine/WorkList.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/Pointer.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/Precision/PrecisionLossTracker.h"
#include "PointerAnalysis/Precision/TrackerGlobalState.h"
#include "PointerAnalysis/Precision/ValueDependenceTracker.h"
#include "PointerAnalysis/Program/SemiSparseProgram.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

using namespace context;
using namespace llvm;

namespace tpa
{

static const Function* getFunction(const Value* val)
{
	if (auto arg = dyn_cast<Argument>(val))
		return arg->getParent();
	else if (auto inst = dyn_cast<Instruction>(val))
		return inst->getParent()->getParent();
	else
		return nullptr;
}

PrecisionLossTracker::ProgramPointList PrecisionLossTracker::getProgramPointsFromPointers(const PointerList& ptrs)
{
	ProgramPointList list;
	list.reserve(ptrs.size());

	for (auto ptr: ptrs)
	{
		auto value = ptr->getValue();

		if (auto func = getFunction(value))
		{
			auto cfg = globalState.getSemiSparseProgram().getCFGForFunction(*func);
			assert(cfg != nullptr);

			auto node = cfg->getCFGNodeForValue(value);
			assert(node != nullptr);
			list.push_back(ProgramPoint(ptr->getContext(), node));
		}

	}

	return list;
}

namespace
{

class ImprecisionTracker
{
private:
	TrackerGlobalState& globalState;

	PtsSet getPtsSet(const Context*, const Value*);
	bool morePrecise(const PtsSet&, const PtsSet&);

	void checkCalleeDependencies(const ProgramPoint&, ProgramPointSet&);
	void checkCallerDependencies(const ProgramPoint&, ProgramPointSet&);
public:
	ImprecisionTracker(TrackerGlobalState& s): globalState(s) {}

	void runOnWorkList(BackwardWorkList& workList);
};

PtsSet ImprecisionTracker::getPtsSet(const Context* ctx, const Value* val)
{
	auto ptr = globalState.getPointerManager().getPointer(ctx, val);
	assert(ptr != nullptr);
	return globalState.getEnv().lookup(ptr);
}

void ImprecisionTracker::runOnWorkList(BackwardWorkList& workList)
{
	while (!workList.empty())
	{
		auto pp = workList.dequeue();
		if (!globalState.insertVisitedLocation(pp))
			continue;

		auto deps = ValueDependenceTracker(globalState.getCallGraph(), globalState.getSemiSparseProgram()).getValueDependencies(pp);

		auto node = pp.getCFGNode();
		if (node->isCallNode())
			checkCalleeDependencies(pp, deps);
		else if (node->isEntryNode())
			checkCallerDependencies(pp, deps);
		
		for (auto const& succ: deps)
			workList.enqueue(succ);
	}
}

bool ImprecisionTracker::morePrecise(const PtsSet& lhs, const PtsSet& rhs)
{
	auto uObj = MemoryManager::getUniversalObject();
	if (rhs.has(uObj))
		return !lhs.has(uObj);

	return lhs.size() < rhs.size();
}

void ImprecisionTracker::checkCalleeDependencies(const ProgramPoint& pp, ProgramPointSet& deps)
{
	assert(pp.getCFGNode()->isCallNode());
	auto callNode = static_cast<const CallCFGNode*>(pp.getCFGNode());
	auto dstVal = callNode->getDest();
	if (dstVal == nullptr)
		return;

	auto dstSet = getPtsSet(pp.getContext(), dstVal);
	assert(!dstSet.empty());

	ProgramPointSet newSet;
	bool needPrecision = false;
	for (auto const& retPoint: deps)
	{
		assert(retPoint.getCFGNode()->isReturnNode());
		auto retNode = static_cast<const ReturnCFGNode*>(retPoint.getCFGNode());
		auto retVal = retNode->getReturnValue();
		assert(retVal != nullptr);

		auto retSet = getPtsSet(retPoint.getContext(), retVal);
		assert(!retSet.empty());
		if (morePrecise(retSet, dstSet))
			needPrecision = true;
		else
			newSet.insert(retPoint);
	}

	if (needPrecision)
	{
		globalState.addImprecisionSource(pp);
		deps.swap(newSet);
	}
}

void ImprecisionTracker::checkCallerDependencies(const ProgramPoint& pp, ProgramPointSet& deps)
{
	// TODO: Finish this
}


}

ProgramPointSet PrecisionLossTracker::trackImprecision(const PointerList& ptrs)
{
	ProgramPointSet ppSet;

	auto ppList = getProgramPointsFromPointers(ptrs);
	auto workList = BackwardWorkList();
	for (auto const& pp: ppList)
		workList.enqueue(pp);

	TrackerGlobalState trackerState(globalState.getPointerManager(), globalState.getMemoryManager(), globalState.getSemiSparseProgram(), globalState.getEnv(), globalState.getCallGraph() ,globalState.getExternalPointerTable(), ppSet);
	ImprecisionTracker(trackerState).runOnWorkList(workList);

	return ppSet;
}

}
