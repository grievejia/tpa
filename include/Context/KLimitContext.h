#pragma once

#include "Context/Context.h"

namespace llvm
{
	class Instruction;
}

namespace context
{

class ProgramPoint;

class KLimitContext
{
private:
	static unsigned defaultLimit;
public:
	static void setLimit(unsigned k) { defaultLimit = k; }
	static unsigned getLimit() { return defaultLimit; }

	static const Context* pushContext(const Context*, const llvm::Instruction*);
	static const Context* pushContext(const ProgramPoint&);
};

}
