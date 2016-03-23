#pragma once

#include "Context/Context.h"
#include "Context/ProgramPoint.h"

#include <unordered_set>

namespace context
{

class AdaptiveContext
{
private:
	static std::unordered_set<ProgramPoint> trackedCallsites;
public:
	static void trackCallSite(const ProgramPoint&);

	static const Context* pushContext(const Context*, const llvm::Instruction*);
	static const Context* pushContext(const ProgramPoint&);
};

}
