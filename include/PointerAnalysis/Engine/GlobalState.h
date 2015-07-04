#pragma once

#include "PointerAnalysis/Engine/StorePruner.h"
#include "PointerAnalysis/Support/CallGraph.h"
#include "PointerAnalysis/Support/Env.h"

namespace annotation
{
	class ExternalPointerTable;
}

namespace tpa
{

class MemoryManager;
class PointerManager;
class SemiSparseProgram;

class GlobalState
{
private:
	PointerManager& ptrManager;
	MemoryManager& memManager;

	const SemiSparseProgram& prog;
	const annotation::ExternalPointerTable& extTable;

	Env& env;
	CallGraph callGraph;

	StorePruner pruner;
public:
	GlobalState(PointerManager& p, MemoryManager& m, const SemiSparseProgram& s, const annotation::ExternalPointerTable& t, Env& e): ptrManager(p), memManager(m), prog(s), extTable(t), env(e), pruner(env, ptrManager, memManager) {}

	PointerManager& getPointerManager() { return ptrManager; }
	const PointerManager& getPointerManager() const { return ptrManager; }
	MemoryManager& getMemoryManager() { return memManager; }
	const MemoryManager& getMemoryManager() const { return memManager; }
	const SemiSparseProgram& getSemiSparseProgram() const { return prog; }
	const annotation::ExternalPointerTable& getExternalPointerTable() const { return extTable; }

	Env& getEnv() { return env; }
	const Env& getEnv() const { return env; }

	CallGraph& getCallGraph() { return callGraph; }
	StorePruner& getStorePruner() { return pruner; }
};

}
