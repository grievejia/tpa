#ifndef TPA_TUNABLE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_H

#include "MemoryModel/PtsSet/Env.h"
#include "TPA/DataFlow/Memo.h"
#include "TPA/DataFlow/StaticCallGraph.h"
#include "MemoryModel/PtsSet/StoreManager.h"
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

class TunablePointerAnalysis
{
private:
	PointerManager ptrManager;
	std::unique_ptr<MemoryManager> memManager;
	PtsSetManager pSetManager;
	StoreManager storeManager;

	Env env;
	Memo memo;

	StaticCallGraph callGraph;
public:
	TunablePointerAnalysis();
	~TunablePointerAnalysis();

	void runOnModule(const llvm::Module& module);
};

}

#endif
