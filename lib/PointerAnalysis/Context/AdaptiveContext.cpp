#include "Context/AdaptiveContext.h"

using namespace llvm;

namespace context
{

void AdaptiveContext::trackCallSite(const ProgramPoint& pLoc)
{
	trackedCallsites.insert(pLoc);
}

const Context* AdaptiveContext::pushContext(const Context* ctx, const llvm::Instruction* inst)
{
	return pushContext(ProgramPoint(ctx, inst));
}

const Context* AdaptiveContext::pushContext(const ProgramPoint& pp)
{
	if (trackedCallsites.count(pp))
		return Context::pushContext(pp);
	else
		return pp.getContext();
}

}
