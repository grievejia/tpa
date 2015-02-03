#ifndef TPA_TUNABLE_POINTER_ANALYSIS_H
#define TPA_TUNABLE_POINTER_ANALYSIS_H

#include "DataFlow/Env.h"
#include "DataFlow/Memo.h"
#include "DataFlow/StaticCallGraph.h"
#include "DataFlow/StoreManager.h"
#include "Memory/PointerManager.h"

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
	VectorSetManager<const MemoryLocation*> pSetManager;
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
