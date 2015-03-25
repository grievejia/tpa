#include "MemoryModel/Precision/AdaptiveContext.h"

using namespace llvm;

namespace tpa
{

std::unordered_set<ProgramLocation> AdaptiveContext::trackedCallsites;

void AdaptiveContext::trackCallSite(const ProgramLocation& pLoc)
{
	trackedCallsites.insert(pLoc);
}

const Context* AdaptiveContext::pushContext(const Context* ctx, const llvm::Instruction* inst, const llvm::Function* f)
{
	if (trackedCallsites.count(ProgramLocation(ctx, inst)))
		return Context::pushContext(ctx, inst);
	else
		return ctx;
}

}
