#ifndef TPA_K_LIMIT_CONTEXT_H
#define TPA_K_LIMIT_CONTEXT_H

#include "MemoryModel/Precision/Context.h"

namespace llvm
{
	class Instruction;
}

namespace tpa
{

class KLimitContext
{
public:
	static unsigned defaultLimit;

	static const Context* pushContext(const Context*, const llvm::Instruction*, const llvm::Function*);
};

}

#endif
