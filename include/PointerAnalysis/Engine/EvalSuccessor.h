#pragma once

#include "PointerAnalysis/Support/ProgramPoint.h"

namespace tpa
{

class EvalSuccessor
{
private:
	ProgramPoint pp;
	bool topFlag;

	EvalSuccessor(const ProgramPoint& p, bool top): pp(p), topFlag(top) {}
public:
	bool isTopLevel() const { return topFlag; }
	const ProgramPoint& getProgramPoint() const { return pp; }

	friend class EvalResult;
};

}
