#ifndef TPA_UTIL_DUMMY_POINTER_ANALYSIS_H
#define TPA_UTIL_DUMMY_POINTER_ANALYSIS_H

#include "PointerAnalysis/Analysis/PointerAnalysis.h"

namespace tpa
{

// A dummy pointer analysis which is mostly useful for unit test
class DummyPointerAnalysis: public PointerAnalysis
{
public:
	DummyPointerAnalysis(PointerManager& pm, MemoryManager& mm, const ExternalPointerEffectTable& t): PointerAnalysis(pm, mm, t) {}

	void injectEnv(const Pointer* ptr, const MemoryLocation* loc)
	{
		env.insert(ptr, loc);
	}
};

}

#endif
