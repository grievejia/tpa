#pragma once

#include "PointerAnalysis/FrontEnd/Type/TypeMap.h"
#include "PointerAnalysis/Support/Env.h"
#include "PointerAnalysis/Support/Store.h"

namespace context
{
	class Context;
}

namespace llvm
{
	class Constant;
	class ConstantExpr;
	class DataLayout;
	class GlobalValue;
	class GlobalVariable;
	class Module;
}

namespace tpa
{

class MemoryManager;
class MemoryObject;
class PointerManager;

class GlobalPointerAnalysis
{
private:
	using EnvStore = std::pair<Env, Store>;

	PointerManager& ptrManager;
	MemoryManager& memManager;
	const TypeMap& typeMap;
	const context::Context* globalCtx;

	const MemoryObject* getGlobalObject(const llvm::GlobalValue*, const Env&);

	void initializeSpecialPointerObject(const llvm::Module&, EnvStore&);
	void createGlobalVariables(const llvm::Module&, Env&);
	void createFunctions(const llvm::Module&, Env&);

	void initializeGlobalValues(const llvm::Module&, EnvStore&);
	void processGlobalInitializer(const MemoryObject*, const llvm::Constant*, EnvStore&, const llvm::DataLayout&);
	void processGlobalScalarInitializer(const MemoryObject*, const llvm::Constant*, EnvStore&, const llvm::DataLayout&);
	void processGlobalStructInitializer(const MemoryObject*, const llvm::Constant*, EnvStore&, const llvm::DataLayout&);
	void processGlobalArrayInitializer(const MemoryObject*, const llvm::Constant*, EnvStore&, const llvm::DataLayout&);
	std::pair<const llvm::GlobalVariable*, size_t> processConstantGEP(const llvm::ConstantExpr*, const llvm::DataLayout&);
public:
	GlobalPointerAnalysis(PointerManager&, MemoryManager&, const TypeMap&);

	EnvStore runOnModule(const llvm::Module&);
};

}