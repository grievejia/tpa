#include "Context/KLimitContext.h"
#include "Context/ProgramPoint.h"

using namespace llvm;

namespace context
{

const Context* KLimitContext::pushContext(const ProgramPoint& pp)
{
	return pushContext(pp.getContext(), pp.getInstruction());
}

const Context* KLimitContext::pushContext(const Context* ctx, const Instruction* inst)
{
	size_t k = defaultLimit;
	
	assert(ctx->size() <= k);
	if (ctx->size() == k)
		return ctx;
	else
		return Context::pushContext(ctx, inst);
}

}
