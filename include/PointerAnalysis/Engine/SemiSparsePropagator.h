#pragma once

#include "PointerAnalysis/Engine/StorePruner.h"

namespace tpa
{

class EvalResult;
class GlobalState;
class Memo;
class WorkList;

class SemiSparsePropagator
{
private:
	Memo& memo;
	WorkList& workList;

	void propagateEntryNode(const ProgramPoint&, const Store&);
	void enqueueIfMemoChange(const ProgramPoint&, const Store&);
public:
	SemiSparsePropagator(Memo& m, WorkList& w): memo(m), workList(w) {}

	void propagate(const EvalResult&);
};

}
