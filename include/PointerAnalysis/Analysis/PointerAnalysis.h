#ifndef TPA_POINTER_ANALYSIS_H
#define TPA_POINTER_ANALYSIS_H

#include "MemoryModel/PtsSet/Env.h"
#include "MemoryModel/PtsSet/PtsSet.h"
#include "PointerAnalysis/ControlFlow/StaticCallGraph.h"

namespace llvm
{
	class Function;
	class Instruction;
	class Value;
}

namespace tpa
{

class ExternalPointerTable;
class MemoryManager;
class PointerManager;

class PointerAnalysis
{
protected:
	PointerManager& ptrManager;
	MemoryManager& memManager;

	const ExternalPointerTable& extTable;

	Env env;
	StaticCallGraph callGraph;
public:
	// Constructor with default env
	PointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerTable& e): ptrManager(p), memManager(m), extTable(e), env() {}
	// Constructors with user-initialized env
	PointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerTable& ext, const Env& e): ptrManager(p), memManager(m), extTable(ext), env(e) {}
	PointerAnalysis(PointerManager& p, MemoryManager& m, const ExternalPointerTable& ext, Env&& e): ptrManager(p), memManager(m), extTable(ext), env(std::move(e)) {}
	virtual ~PointerAnalysis() = default;

	PtsSet getPtsSet(const llvm::Value* val) const;
	PtsSet getPtsSet(const Context*, const llvm::Value*) const;
	PtsSet getPtsSet(const Pointer* ptr) const;

	const MemoryManager& getMemoryManager() const { return memManager; }
	const StaticCallGraph& getCallGraph() const { return callGraph; }
	std::vector<const llvm::Function*> getCallTargets(const Context*, const llvm::Instruction*) const;
};

}

#endif
