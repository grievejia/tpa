#ifndef TPA_K_LIMIT_CONTEXT_H
#define TPA_K_LIMIT_CONTEXT_H

#include "MemoryModel/Precision/Context.h"

#include <llvm/ADT/DenseSet.h>

namespace llvm
{
	class Instruction;
}

namespace tpa
{

class KLimitContext
{
private:
	static llvm::DenseSet<const llvm::Instruction*> kSet;
public:
	static unsigned defaultLimit;
	static void trackContext(const llvm::Instruction*);

	static const Context* pushContext(const Context*, const llvm::Instruction*, const llvm::Function*);
};

}

#endif
