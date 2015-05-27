#include "MemoryModel/Precision/KLimitContext.h"

#include <llvm/Support/CommandLine.h>

using namespace llvm;

static cl::opt<unsigned, true> KLimitContextSensitivity("k", cl::desc("In k-limit stack-k-CFA settings, this is the value of k"), cl::location(tpa::KLimitContext::defaultLimit), cl::init(0));

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
