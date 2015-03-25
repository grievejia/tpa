#ifndef TPA_ADAPTIVE_CONTEXT_H
#define TPA_ADAPTIVE_CONTEXT_H

#include "MemoryModel/Precision/ProgramLocation.h"

#include <unordered_set>

namespace tpa
{

class AdaptiveContext
{
private:
	static std::unordered_set<ProgramLocation> trackedCallsites;
public:
	static void trackCallSite(const ProgramLocation& pLoc);

	static const Context* pushContext(const Context*, const llvm::Instruction*, const llvm::Function*);
};

}

#endif
