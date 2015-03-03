#include "MemoryModel/Precision/KLimitContext.h"

using namespace llvm;

namespace tpa
{

DenseSet<const Instruction*> KLimitContext::kSet;
unsigned KLimitContext::defaultLimit = 0u;

void KLimitContext::trackContext(const Instruction* i)
{
	kSet.insert(i);
}

const Context* KLimitContext::pushContext(const Context* ctx, const Instruction* inst, const Function* f)
{
	size_t k = defaultLimit;
	//if (kSet.count(inst))
	//	return Context::pushContext(ctx, inst);
	//else
	//	return ctx;
	
	assert(ctx->getSize() <= k);
	if (ctx->getSize() == k)
		return ctx;
	else
		return Context::pushContext(ctx, inst);
}

}
