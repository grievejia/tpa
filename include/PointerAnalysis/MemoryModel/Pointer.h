#pragma once

#include "Util/Hashing.h"

#include <cassert>

namespace context
{
	class Context;
}

namespace llvm
{
	class Value;
}

namespace tpa
{

// The Pointer class models SSA pointers
class Pointer
{
private:
	const context::Context* ctx;
	const llvm::Value* value;

	using PairType = std::pair<const context::Context*, const llvm::Value*>;

	Pointer(const context::Context* c, const llvm::Value* v): ctx(c), value(v)
	{
		assert(c != nullptr && v != nullptr);
	}
public:
	const context::Context* getContext() const { return ctx; }
	const llvm::Value* getValue() const { return value; }

	bool operator==(const Pointer& rhs) const
	{
		return ctx == rhs.ctx && value == rhs.value;
	}
	bool operator!=(const Pointer& rhs) const
	{
		return !(*this == rhs);
	}

	operator PairType() const
	{
		return std::make_pair(ctx, value);
	}

	friend class PointerManager;
};

}

namespace std
{

template<> struct hash<tpa::Pointer>
{
	size_t operator()(const tpa::Pointer& ptr) const
	{
		return util::hashPair(ptr.getContext(), ptr.getValue());
	}
};

}
