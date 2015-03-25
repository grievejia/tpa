#include "MemoryModel/Precision/KLimitContext.h"

using namespace llvm;

namespace tpa
{

unsigned KLimitContext::defaultLimit = 0u;

const Context* KLimitContext::pushContext(const Context* ctx, const Instruction* inst, const Function* f)
{
	size_t k = defaultLimit;
	
	assert(ctx->getSize() <= k);
	if (ctx->getSize() == k)
		return ctx;
	else
		return Context::pushContext(ctx, inst);
}

}
