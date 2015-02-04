#ifndef TPA_GLOBAL_POINTER_ANALYSIS_H
#define TPA_GLOBAL_POINTER_ANALYSIS_H

#include "MemoryModel/PtsSet/Env.h"
#include "MemoryModel/PtsSet/Store.h"

namespace llvm
{
	class Constant;
	class DataLayout;
	class Module;
}

namespace tpa
{

class MemoryLocation;
class MemoryManager;
class PointerManager;
class StoreManager;

class GlobalPointerAnalysis
{
private:
	PointerManager& ptrManager;
	MemoryManager& memManager;
	StoreManager& storeManager;

	const llvm::DataLayout& dataLayout;
	const Context* globalCtx;

	void registerGlobalValues(Env&, const llvm::Module&);
	void initializeGlobalValues(Env&, Store&, const llvm::Module&);
	void initializeMainArgs(const llvm::Module&, Env&, Store&);
	void processGlobalInitializer(const MemoryLocation*, const llvm::Constant*, const Env&, Store&);
	const MemoryLocation* processConstantGEP(const llvm::Constant*, size_t, const Env&, Store&);
public:
	GlobalPointerAnalysis(PointerManager& p, MemoryManager& m, StoreManager& s);

	std::pair<Env, Store> runOnModule(const llvm::Module&);
};

}

#endif
