#ifndef TPA_TUNABLE_ALIAS_ANALYSIS_H
#define TPA_TUNABLE_ALIAS_ANALYSIS_H

#include "TPA/Analysis/TunablePointerAnalysis.h"

namespace llvm
{
	class Module;
	class DataLayout;
}

namespace tpa
{

class MemoryLocation;
class MemoryManager;

enum class AliasResult
{
	NoAlias,
	MayAlias,
	MustAlias,
};

class TunableAliasAnalysis
{
private:
	TunablePointerAnalysis tpaAnalysis;
	const MemoryLocation *uLoc, *nLoc;

	AliasResult checkAlias(const PtsSet*, const PtsSet*);
public:
	TunableAliasAnalysis() = default;

	void runOnModule(const llvm::Module& module);

	AliasResult aliasQuery(const Pointer*, const Pointer*);
	AliasResult globalAliasQuery(const llvm::Value*, const llvm::Value*);
};

}

#endif
