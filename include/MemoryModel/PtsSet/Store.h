#ifndef TPA_STORE_H
#define TPA_STORE_H

#include "MemoryModel/PtsSet/PtsMap.h"

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

using Store = PtsMap<const MemoryLocation*>;

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Store& store);

}

#endif
