#ifndef TPA_TAINT_PRECISION_MONITOR_H
#define TPA_TAINT_PRECISION_MONITOR_H

#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "Client/Taintness/Precision/PrecisionLossRecord.h"

namespace client
{
namespace taint
{

class TaintPrecisionMonitor
{
private:
	using TaintLatticeVector = std::vector<TaintLattice>;

	const TaintEnv& env;

	PrecisionLossRecord record;
public:
	TaintPrecisionMonitor(const TaintEnv& e);

	void trackPrecisionLoss(const tpa::ProgramLocation&, const tpa::Context*, const llvm::Function*, const TaintLatticeVector&);

	const PrecisionLossRecord& getPrecisionLossRecord() const
	{
		return record;
	}
};

}
}

#endif
