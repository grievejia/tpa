#include "Precision/Context.h"

using namespace llvm;

namespace tpa
{

std::unordered_set<Context> Context::ctxSet;

const Context* Context::pushContext(const Context* ctx, const Instruction* inst)
{
	auto newCtx = Context(inst, ctx);
	auto itr = ctxSet.insert(newCtx).first;
	return &(*itr);
}

const Context* Context::popContext(const Context* ctx)
{
	assert(ctx->size != 0 && "Trying to pop an empty context");
	return ctx->predContext;
}

const Context* Context::getGlobalContext()
{
	auto itr = ctxSet.insert(Context()).first;
	return &(*itr);
}

}
