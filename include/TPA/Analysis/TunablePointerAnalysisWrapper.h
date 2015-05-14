#ifndef TPA_SEMI_SPARSE_POINTER_ANALYSIS_WRAPPER_H
#define TPA_SEMI_SPARSE_POINTER_ANALYSIS_WRAPPER_H

#include "MemoryModel/Pointer/PointerManager.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "TPA/Analysis/TunablePointerAnalysis.h"

#include <memory>

namespace llvm
{
	class Module;
}

namespace tpa
{

class MemoryManager;

class TunablePointerAnalysisWrapper
{
private:
	PointerManager ptrManager;
	std::unique_ptr<MemoryManager> memManager;

	PointerProgram prog;
	std::unique_ptr<TunablePointerAnalysis> ptrAnalysis;

	ExternalPointerTable extTable;
public:
	TunablePointerAnalysisWrapper();
	~TunablePointerAnalysisWrapper();

	const ExternalPointerTable& getExtTable() const { return extTable; }
	const PointerProgram& getPointerProgram() const { return prog; }
	const TunablePointerAnalysis& getPointerAnalysis() const 
	{
		assert(ptrAnalysis.get() != nullptr);
		return *ptrAnalysis;
	}

	void runOnModule(const llvm::Module& module);
};

}

#endif
