#pragma once

#include "TaintAnalysis/Support/ProgramPoint.h"
#include "Util/Hashing.h"

namespace llvm
{
	class Function;
}

namespace taint
{

class SinkSignature
{
private:
	ProgramPoint callsite;
	const llvm::Function* callee;
public:
	SinkSignature(const ProgramPoint& p, const llvm::Function* f): callsite(p), callee(f) {}

	const ProgramPoint& getCallSite() const
	{
		return callsite;
	}

	const llvm::Function* getCallee() const
	{
		return callee;
	}

	bool operator==(const SinkSignature& rhs) const
	{
		return callee == rhs.callee && callsite == rhs.callsite;
	}
	bool operator!=(const SinkSignature& rhs) const
	{
		return !(*this == rhs);
	}
};

}

namespace std
{
	template<> struct hash<taint::SinkSignature>
	{
		size_t operator()(const taint::SinkSignature& s) const
		{
			return util::hashPair(s.getCallSite(), s.getCallee());
		}
	};
}