#ifndef TPA_TAINT_PRECISION_LOSS_GLOBAL_STATE_H
#define TPA_TAINT_PRECISION_LOSS_GLOBAL_STATE_H

#include "PointerAnalysis/DataFlow/DefUseProgramLocation.h"

#include <unordered_set>

namespace client
{
namespace taint
{

using DefUseProgramLocationSet = std::unordered_set<tpa::DefUseProgramLocation>;

class PrecisionLossGlobalState
{
private:
	DefUseProgramLocationSet& imprecisionSources;
	DefUseProgramLocationSet visited;
public:
	PrecisionLossGlobalState(DefUseProgramLocationSet& s): imprecisionSources(s) {}

	// Return true if the visited set changes
	bool insertVisitedLocation(const tpa::DefUseProgramLocation& pLoc)
	{
		return visited.insert(pLoc).second;
	}

	void addImprecisionSource(const tpa::DefUseProgramLocation& pLoc)
	{
		imprecisionSources.insert(pLoc);
	}
};

}
}

#endif
