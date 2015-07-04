#pragma once

#include "Util/Hashing.h"

#include <unordered_set>

namespace dynamic
{

// Dynamic version of Context
class DynamicContext
{
private:
	using CallSiteType = unsigned;

	CallSiteType callSite;
	const DynamicContext* predContext;
	size_t depth;

	static std::unordered_set<DynamicContext> ctxSet;
	static const DynamicContext* globalCtx;

	DynamicContext(): callSite(0), predContext(nullptr), depth(0) {}
	DynamicContext(CallSiteType c, const DynamicContext* p): callSite(c), predContext(p), depth(p == nullptr ? 1 : p->depth + 1) {}
public:
	CallSiteType getCallSite() const { return callSite; }
	size_t getDepth() const { return depth; }
	bool isGlobalContext() const { return depth == 0; }

	bool operator==(const DynamicContext& other) const
	{
		return callSite == other.callSite && predContext == other.predContext;
	}
	bool operator!=(const DynamicContext& other) const
	{
		return !(*this == other);
	}

	static const DynamicContext* pushContext(const DynamicContext* ctx, CallSiteType cs);
	static const DynamicContext* popContext(const DynamicContext* ctx);
	static const DynamicContext* getGlobalContext();

	friend struct std::hash<DynamicContext>;
};

}

namespace std
{

template<> struct hash<dynamic::DynamicContext>
{
	size_t operator()(const dynamic::DynamicContext& c) const
	{
		return util::hashPair(c.callSite, c.predContext);
	}
};

}
