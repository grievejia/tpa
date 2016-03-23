#pragma once

#include "PointerAnalysis/Support/CallGraph.h"
#include "PointerAnalysis/Support/FunctionContext.h"
#include "PointerAnalysis/Support/ProgramPointSet.h"

#include <vector>

namespace tpa
{

class SemiSparseProgram;

class ValueDependenceTracker
{
private:
	using CallGraphType = CallGraph<ProgramPoint, FunctionContext>;
	const CallGraphType& callGraph;

	const SemiSparseProgram& ssProg;
public:
	ValueDependenceTracker(const CallGraphType& c, const SemiSparseProgram& s): callGraph(c), ssProg(s) {}

	ProgramPointSet getValueDependencies(const ProgramPoint&) const;
};

}
