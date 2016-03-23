#pragma once

#include "Util/Hashing.h"

#include <cassert>

namespace context
{
	class Context;
}

namespace tpa
{

class CFGNode;

class ProgramPoint
{
private:
	const context::Context* ctx;
	const CFGNode* node;
public:
	ProgramPoint(const context::Context* c, const CFGNode* n): ctx(c), node(n)
	{
		assert(ctx != nullptr && node != nullptr);
	}

	const context::Context* getContext() const { return ctx; }
	const CFGNode* getCFGNode() const { return node; }

	bool operator==(const ProgramPoint& rhs) const
	{
		return ctx == rhs.ctx && node == rhs.node;
	}
	bool operator!=(const ProgramPoint& rhs) const
	{
		return !(*this == rhs);
	}
	bool operator<(const ProgramPoint& rhs) const
	{
		if (ctx < rhs.ctx)
			return true;
		else if (rhs.ctx < ctx)
			return false;
		else
			return node < rhs.node;
	}
	bool operator>(const ProgramPoint& rhs) const
	{
		return rhs < *this;
	}
	bool operator<=(const ProgramPoint& rhs) const
	{
		return !(rhs < *this);
	}
	bool operator>=(const ProgramPoint& rhs) const
	{
		return !(*this < rhs);
	}
};

}

namespace std
{
	template<> struct hash<tpa::ProgramPoint>
	{
		size_t operator()(const tpa::ProgramPoint& p) const
		{
			return util::hashPair(p.getContext(), p.getCFGNode());
		}
	};
}
