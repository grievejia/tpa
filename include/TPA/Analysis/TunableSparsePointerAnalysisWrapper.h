#ifndef TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_WRAPPER_H
#define TPA_TUNABLE_SPARSE_POINTER_ANALYSIS_WRAPPER_H

#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/DataFlow/DefUseProgram.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/Analysis/TunableSparsePointerAnalysis.h"

#include <memory>

namespace llvm
{
	class Module;
}

namespace tpa
{

class MemoryManager;

class TunableSparsePointerAnalysisWrapper
{
private:
	PointerManager ptrManager;
	std::unique_ptr<MemoryManager> memManager;

	DefUseProgram dug;
	std::unique_ptr<TunableSparsePointerAnalysis> ptrAnalysis;

	ExternalPointerEffectTable extTable;
public:
	TunableSparsePointerAnalysisWrapper();
	~TunableSparsePointerAnalysisWrapper();

	const ExternalPointerEffectTable& getExtTable() const { return extTable; }
	const DefUseProgram& getDefUseProgram() const { return dug; }
	const TunableSparsePointerAnalysis& getPointerAnalysis() const 
	{
		assert(ptrAnalysis.get() != nullptr);
		return *ptrAnalysis;
	}

	void runOnModule(const llvm::Module& module);
};

}

#endif
