#ifndef TPA_TAINT_SINK_SIGNATURE_H
#define TPA_TAINT_SINK_SIGNATURE_H

#include "PointerAnalysis/DataFlow/DefUseProgramLocation.h"

namespace llvm
{
	class Function;
}

namespace client
{
namespace taint
{

class SinkSignature
{
private:
	tpa::DefUseProgramLocation callsite;
	const llvm::Function* callee;
public:
	SinkSignature(const tpa::DefUseProgramLocation& p, const llvm::Function* f): callsite(p), callee(f) {}

	const tpa::DefUseProgramLocation& getCallSite() const
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
}

namespace std
{
	template<> struct hash<client::taint::SinkSignature>
	{
		size_t operator()(const client::taint::SinkSignature& s) const
		{
			std::size_t seed = 0;
			tpa::hash_combine(seed, s.getCallSite());
			tpa::hash_combine(seed, s.getCallee());
			return seed;
		}
	};
}

#endif
