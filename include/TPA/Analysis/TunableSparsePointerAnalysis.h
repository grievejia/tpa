#ifndef TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_H

#include "MemoryModel/Pointer/PointerManager.h"
#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/StaticCallGraph.h"

namespace tpa
{

class DefUseGraphNode;
class MemoryManager;

class TunableSparsePointerAnalysis: public PointerAnalysis
{
private:
	PointerManager ptrManager;
	std::unique_ptr<MemoryManager> memManager;
	PtsSetManager pSetManager;
	StoreManager storeManager;

	Env env;
	Memo<DefUseGraphNode> memo;

	StaticCallGraph callGraph;
public:
	TunableSparsePointerAnalysis();
	~TunableSparsePointerAnalysis();

	void runOnModule(const llvm::Module& module);

	const MemoryManager& getMemoryManager() const override
	{
		assert(memManager != nullptr);
		return *memManager;
	}
	const PtsSet* getPtsSet(const llvm::Value* val) const override;
	const PtsSet* getPtsSet(const Pointer* ptr) const override;
	std::vector<const llvm::Function*> getCallTargets(const Context*, const llvm::Instruction*) const override;
};

}

#endif
