#ifndef TPA_TAINT_STORE_H
#define TPA_TAINT_STORE_H

#include "Client/Taintness/DataFlow/TaintMapping.h"

namespace tpa
{
	class MemoryLocation;
}

namespace client
{
namespace taint
{

using TaintStore = TaintMap<const tpa::MemoryLocation*>;

}
}

#endif
