#ifndef TPA_TAINT_SINK_VIOLATION_RECORD_H
#define TPA_TAINT_SINK_VIOLATION_RECORD_H

#include "Client/Lattice/TaintLattice.h"
#include "Client/Taintness/SourceSink/Table/TaintDescriptor.h"

namespace client
{
namespace taint
{

struct SinkViolationRecord
{
	uint8_t argPos;
	TClass what;
	TaintLattice expectVal;
	TaintLattice actualVal;
};

using SinkViolationRecords = std::vector<SinkViolationRecord>;

}
}

#endif