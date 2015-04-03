#ifndef TPA_TAINT_PRECISION_MONITOR_H
#define TPA_TAINT_PRECISION_MONITOR_H

#include "Client/Lattice/TaintLattice.h"
#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "MemoryModel/Precision/ProgramLocation.h"

#include <unordered_set>
#include <vector>

namespace llvm
{
	class Function;
}

namespace client
{
namespace taint
{

class TaintPrecisionMonitor
{
private:
	using TaintLatticeList = std::vector<TaintLattice>;

	std::unordered_set<tpa::ProgramLocation> trackedCallSite;
public:
	using const_iterator = decltype(trackedCallSite)::const_iterator;

	TaintPrecisionMonitor() = default;

	void trackCallSite(const tpa::ProgramLocation& pLoc, const llvm::Function* callee, const tpa::Context* newCtx, const TaintEnv& env, const TaintLatticeList& argList);

	void changeContextSensitivity() const;

	const_iterator begin() const { return trackedCallSite.begin(); }
	const_iterator end() const { return trackedCallSite.end(); }
};

}
}

#endif
