#pragma once

#include "Util/Hashing.h"

namespace dynamic
{

class DynamicContext;

class DynamicPointer
{
private:
	const DynamicContext* ctx;
	unsigned ptrId;
public:
	DynamicPointer(const DynamicContext* c, unsigned p): ctx(c), ptrId(p) {}

	bool operator==(const DynamicPointer& rhs) const
	{
		return ctx == rhs.ctx && ptrId == rhs.ptrId;
	}
	bool operator!=(const DynamicPointer& rhs) const
	{
		return !(*this == rhs);
	}

	const DynamicContext* getContext() const { return ctx; }
	unsigned getID() const { return ptrId; }
};

}

namespace std
{

template<> struct hash<dynamic::DynamicPointer>
{
	size_t operator()(const dynamic::DynamicPointer& ptr) const
	{
		return util::hashPair(ptr.getContext(), ptr.getID());
	}
};

}
