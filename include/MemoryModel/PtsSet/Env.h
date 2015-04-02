#ifndef TPA_PTS_ENV_H
#define TPA_PTS_ENV_H

#include "MemoryModel/Pointer/Pointer.h"
#include "MemoryModel/PtsSet/PtsMap.h"

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

using Env = PtsMap<const Pointer*>;

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Env& env);

}

#endif
