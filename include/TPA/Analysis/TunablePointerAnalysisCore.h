#ifndef TPA_TUNABLE_POINTER_ANALYSIS_CORE_H
#define TPA_TUNABLE_POINTER_ANALYSIS_CORE_H

#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/StaticCallGraph.h"

namespace llvm
{
	class Module;
	class DataLayout;
}

namespace tpa
{

class Env;
class MemoryManager;
class PointerManager;
class PointerProgram;
class StoreManager;

class TunablePointerAnalysisCore: public PointerAnalysis
{
private:
	PointerManager& ptrManager;
	MemoryManager& memManager;
	StoreManager& storeManager;

	const ExternalPointerEffectTable& extTable;

	Env env;
	Memo<PointerCFGNode> memo;

	StaticCallGraph callGraph;
public:
	TunablePointerAnalysisCore(PointerManager& p, MemoryManager& m, StoreManager& sm, const Env& e, ExternalPointerEffectTable& ex);

	void runOnProgram(const PointerProgram& prog, const Store& store);

	const MemoryManager& getMemoryManager() const override
	{
		return memManager;
	}
	const PtsSet* getPtsSet(const llvm::Value* val) const override;
	const PtsSet* getPtsSet(const Pointer* ptr) const override;
	std::vector<const llvm::Function*> getCallTargets(const Context*, const llvm::Instruction*) const override;
};

}

#endif
