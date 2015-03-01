#ifndef TPA_K_LIMIT_CONTEXT_H
#define TPA_K_LIMIT_CONTEXT_H

#include "MemoryModel/Precision/Context.h"

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class Function;
}

namespace tpa
{

class KLimitContext
{
private:
	static llvm::DenseMap<const llvm::Function*, size_t> kMap;
public:
	static unsigned defaultLimit;
	static void setLimit(const llvm::Function*, size_t);

	static const Context* pushContext(const Context*, const llvm::Instruction*, const llvm::Function*);
};

}

#endif
