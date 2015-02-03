#include "Precision/KLimitContext.h"

using namespace llvm;

namespace tpa
{

DenseMap<const Function*, size_t> KLimitContext::kMap;

void KLimitContext::setLimit(const Function* f, size_t k)
{
	kMap[f] = k;
}

const Context* KLimitContext::pushContext(const Context* ctx, const Instruction* inst, const Function* f)
{
	size_t k = 0;
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
