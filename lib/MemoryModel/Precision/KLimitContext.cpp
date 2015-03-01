#include "MemoryModel/Precision/KLimitContext.h"

using namespace llvm;

namespace tpa
{

DenseMap<const Function*, size_t> KLimitContext::kMap;
unsigned KLimitContext::defaultLimit = 0u;

void KLimitContext::setLimit(const Function* f, size_t k)
{
	kMap[f] = k;
}

const Context* KLimitContext::pushContext(const Context* ctx, const Instruction* inst, const Function* f)
{
	size_t k = defaultLimit;
	auto itr = kMap.find(f);
	if (itr != kMap.end())
		k = itr->second;
	
	assert(ctx->getSize() <= k);
	if (ctx->getSize() == k)
		return ctx;
	else
		return Context::pushContext(ctx, inst);
}

}
