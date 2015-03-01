#ifndef TPA_TUNABLE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_H

#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/StaticCallGraph.h"

#include <memory>

namespace llvm
{
	class Module;
	class DataLayout;
}

namespace tpa
{

class MemoryManager;

class TunablePointerAnalysis: public PointerAnalysis
{
private:
	PointerManager ptrManager;
	std::unique_ptr<MemoryManager> memManager;
	mutable PtsSetManager pSetManager;
	StoreManager storeManager;

	PointerProgram prog;

	Env env;
	Memo<PointerCFGNode> memo;

	StaticCallGraph callGraph;
public:
	TunablePointerAnalysis();
	~TunablePointerAnalysis();

	const PointerProgram& getPointerProgram() const { return prog; }

	void runOnModule(const llvm::Module& module);

	const MemoryManager& getMemoryManager() const override
	{
		assert(memManager != nullptr);
		return *memManager;
	}
	const PtsSet* getPtsSet(const llvm::Value* val) const override;
	const PtsSet* getPtsSet(const Pointer* ptr) const override;
	const PtsSet* getPtsSet(const Context*, const llvm::Value*) const override;
	std::vector<const llvm::Function*> getCallTargets(const Context*, const llvm::Instruction*) const override;
};

}

#endif
