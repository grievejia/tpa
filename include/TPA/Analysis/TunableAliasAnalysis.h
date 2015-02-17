#ifndef TPA_TUNABLE_ALIAS_ANALYSIS_H
#define TPA_TUNABLE_ALIAS_ANALYSIS_H

#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/Pointer/PointerManager.h"

#include <memory>

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
	PointerManager ptrManager;
	std::unique_ptr<MemoryManager> memManager;
	PtsSetManager pSetManager;

	Env env;
public:
	TunableAliasAnalysis();
	~TunableAliasAnalysis();

	void runOnModule(const llvm::Module& module);

	AliasResult aliasQuery(const Pointer*, const Pointer*);
	AliasResult globalAliasQuery(const llvm::Value*, const llvm::Value*);
};

}

#endif
