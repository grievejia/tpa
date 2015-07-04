#include "Dynamic/Analysis/DynamicContext.h"

#include <cassert>

namespace dynamic
{

std::unordered_set<DynamicContext> DynamicContext::ctxSet;
const DynamicContext* DynamicContext::globalCtx = &*(ctxSet.insert(DynamicContext()).first);

const DynamicContext* DynamicContext::pushContext(const DynamicContext* ctx, CallSiteType cs)
{
	auto newCtx = DynamicContext(cs, ctx);
	auto itr = ctxSet.insert(newCtx).first;
	return &(*itr);
}

const DynamicContext* DynamicContext::popContext(const DynamicContext* ctx)
{
	assert(ctx->depth != 0 && "Trying to pop an empty context");
	return ctx->predContext;
}

const DynamicContext* DynamicContext::getGlobalContext()
{
	return globalCtx;
}

}
