#pragma once

#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Support/Env.h"
#include "PointerAnalysis/Support/Memo.h"

namespace tpa
{

class SemiSparseProgram;

class SemiSparsePointerAnalysis: public PointerAnalysis<SemiSparsePointerAnalysis>
{
private:
	Env env;
	Memo memo;
public:
	SemiSparsePointerAnalysis() = default;

	void runOnProgram(const SemiSparseProgram&);

	PtsSet getPtsSetImpl(const Pointer*) const;
};

}
