#ifndef TPA_TAINT_ENV_H
#define TPA_TAINT_ENV_H

#include "Client/Taintness/DataFlow/TaintMapping.h"
#include "MemoryModel/Precision/ProgramLocation.h"

namespace client
{
namespace taint
{

class TaintEnv
{
private:
	TaintMap<std::pair<const tpa::Context*, const llvm::Value*>> env;
public:
	using const_iterator = typename decltype(env)::const_iterator;

	TaintEnv() = default;

	TaintLattice lookup(const tpa::ProgramLocation& pLoc) const
	{
		if (llvm::isa<llvm::Constant>(pLoc.getInstruction()))
			return TaintLattice::Untainted;
		else
			return env.lookup(pLoc);
	}

	bool weakUpdate(const tpa::ProgramLocation& pLoc, TaintLattice l)
	{
		assert(!llvm::isa<llvm::Constant>(pLoc.getInstruction()));
		return env.weakUpdate(pLoc, l);
	}

	bool strongUpdate(const tpa::ProgramLocation& pLoc, TaintLattice l)
	{
		assert(!llvm::isa<llvm::Constant>(pLoc.getInstruction()));
		return env.strongUpdate(pLoc, l);
	}

	bool mergeWith(const TaintEnv& rhs)
	{
		return env.mergeWith(rhs.env);
	}

	const_iterator begin() const { return env.begin(); }
	const_iterator end() const { return env.end(); }
	size_t getSize() const { return env.getSize(); }
};

}
}

#endif
