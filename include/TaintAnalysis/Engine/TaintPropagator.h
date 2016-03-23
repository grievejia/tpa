#pragma once

#include "TaintAnalysis/Support/TaintStore.h"

namespace taint
{

class EvalResult;
class ProgramPoint;
class TaintMemo;
class WorkList;

class TaintPropagator
{
private:
	TaintMemo& memo;
	WorkList& workList;

	void enqueueIfMemoChange(const ProgramPoint&, const tpa::MemoryObject*, const TaintStore&);
public:
	TaintPropagator(TaintMemo& m, WorkList& w): memo(m), workList(w) {}

	void propagate(const EvalResult&);
};

}