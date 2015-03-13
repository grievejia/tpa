#ifndef TPA_ALIAS_ANALYSIS_H
#define TPA_ALIAS_ANALYSIS_H

#include "PointerAnalysis/Analysis/PointerAnalysis.h"

namespace tpa
{

class MemoryLocation;
class Pointer;

enum class AliasResult
{
	NoAlias,
	MayAlias,
	MustAlias,
};

class AliasAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const MemoryLocation *uLoc, *nLoc;

	AliasResult checkAlias(const PtsSet*, const PtsSet*);
public:
	AliasAnalysis(const PointerAnalysis& ptrAnalysis);

	AliasResult aliasQuery(const Pointer*, const Pointer*);
	AliasResult globalAliasQuery(const llvm::Value*, const llvm::Value*);
};

}

#endif
