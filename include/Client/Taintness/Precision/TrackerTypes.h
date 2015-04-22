#ifndef TPA_TAINT_TRACKER_TYPES_H
#define TPA_TAINT_TRACKER_TYPES_H

#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "TPA/DataFlow/TPAWorkList.h"

#include <llvm/ADT/SmallPtrSet.h>

namespace llvm
{
	class Value;
}

namespace tpa
{
	class MemoryLocation;
}

namespace client
{
namespace taint
{

using ValueSet = llvm::SmallPtrSet<const llvm::Value*, 8>;
using MemorySet = llvm::SmallPtrSet<const tpa::MemoryLocation*, 8>;

using GlobalWorkList = tpa::TPAWorkList<tpa::DefUseFunction>;
using LocalWorkList = typename GlobalWorkList::LocalWorkList;

}
}

#endif
