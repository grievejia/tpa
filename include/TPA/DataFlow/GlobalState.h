#ifndef TPA_GLOBAL_STATE_H
#define TPA_GLOBAL_STATE_H

#include "MemoryModel/PtsSet/Env.h"
#include "TPA/DataFlow/Memo.h"

namespace tpa
{

class ExternalPointerEffectTable;
class MemoryManager;
class PointerManager;
class PointerProgram;
class StaticCallGraph;

template <typename ProgType, typename NodeType = typename ProgType::NodeType>
class GlobalState
{
private:
	// Managers
	PointerManager& ptrManager;
	MemoryManager& memManager;

	// The program
	const ProgType& prog;

	// Call graph
	StaticCallGraph& callGraph;

	// The environment
	Env& env;

	// The external table
	const ExternalPointerEffectTable& extTable;

	// The global memo
	Memo<NodeType> memo;
public:
	GlobalState(PointerManager& pm, MemoryManager& mm, const ProgType& p, StaticCallGraph& s, Env& e, const ExternalPointerEffectTable& t): ptrManager(pm), memManager(mm), prog(p), callGraph(s), env(e), extTable(t) {}

	PointerManager& getPointerManager() { return ptrManager; }
	const PointerManager& getPointerManager() const { return ptrManager; }

	MemoryManager& getMemoryManager() { return memManager; }
	const MemoryManager& getMemoryManager() const { return memManager; }

	const ProgType& getProgram() const { return prog; }

	StaticCallGraph& getCallGraph() { return callGraph; }
	const StaticCallGraph& getCallGraph() const { return callGraph; }

	Env& getEnv() { return env; }
	const Env& getEnv() const { return env; }

	const ExternalPointerEffectTable& getExternalPointerEffectTable() const { return extTable; }

	Memo<NodeType>& getMemo() { return memo; }
	const Memo<NodeType>& getMemo() const { return memo; }
};

}

#endif
