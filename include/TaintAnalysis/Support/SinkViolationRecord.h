#pragma once

#include "Annotation/Taint/TaintDescriptor.h"
#include "TaintAnalysis/Lattice/TaintLattice.h"
#include "TaintAnalysis/Support/ProgramPoint.h"

#include <unordered_map>
#include <vector>

namespace taint
{

struct SinkViolation
{
	uint8_t argPos;
	annotation::TClass what;
	TaintLattice expectVal;
	TaintLattice actualVal;
};

using SinkViolationList = std::vector<SinkViolation>;
using SinkViolationRecord = std::unordered_map<ProgramPoint, SinkViolationList>;

}
