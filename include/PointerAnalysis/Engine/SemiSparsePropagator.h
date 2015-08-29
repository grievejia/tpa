#pragma once

#include "PointerAnalysis/Engine/WorkList.h"

namespace tpa
{

class EvalResult;
class EvalSuccessor;
class Memo;

class SemiSparsePropagator
{
private:
	Memo& memo;
	ForwardWorkList& workList;

	void propagateTopLevel(const EvalSuccessor&);
	void propagateMemLevel(const EvalSuccessor&);
	bool enqueueIfMemoChange(const ProgramPoint&, const Store&);
public:
	SemiSparsePropagator(Memo& m, ForwardWorkList& w): memo(m), workList(w) {}

	void propagate(const EvalResult&);
};

}
