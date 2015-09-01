#include "Context/Context.h"
#include "Context/ProgramPoint.h"

using namespace llvm;

namespace context
{

const Context* Context::pushContext(const ProgramPoint& pp)
{
	return pushContext(pp.getContext(), pp.getInstruction());
}

const Context* Context::pushContext(const Context* ctx, const Instruction* inst)
{
	auto newCtx = Context(inst, ctx);
	auto itr = ctxSet.insert(newCtx).first;
	return &(*itr);
}

const Context* Context::popContext(const Context* ctx)
{
	assert(ctx->sz != 0 && "Trying to pop an empty context");
	return ctx->predContext;
}

const Context* Context::getGlobalContext()
{
	auto itr = ctxSet.insert(Context()).first;
	return &(*itr);
}

std::vector<const Context*> Context::getAllContexts()
{
	std::vector<const Context*> ret;
	ret.reserve(ctxSet.size());

	for (auto const& ctx: ctxSet)
		ret.push_back(&ctx);

	return ret;
}

}
