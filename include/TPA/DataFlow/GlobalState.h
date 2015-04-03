#ifndef TPA_GLOBAL_STATE_H
#define TPA_GLOBAL_STATE_H

#include "MemoryModel/PtsSet/Env.h"
#include "TPA/DataFlow/Memo.h"

namespace tpa
{

class PointerProgram;
class StaticCallGraph;

template <typename ProgType, typename NodeType = typename ProgType::NodeType>
class GlobalState
{
private:
	// The program
	const ProgType& prog;

	// Call graph
	StaticCallGraph& callGraph;

	// The environment
	Env& env;

	// The global memo
	Memo<NodeType> memo;
public:
	GlobalState(const ProgType& p, StaticCallGraph& s, Env& e): prog(p), callGraph(s), env(e) {}

	const ProgType& getProgram() const { return prog; }

	StaticCallGraph& getCallGraph() { return callGraph; }
	const StaticCallGraph& getCallGraph() const { return callGraph; }

	Env& getEnv() { return env; }
	const Env& getEnv() const { return env; }

	Memo<NodeType>& getMemo() { return memo; }
	const Memo<NodeType>& getMemo() const { return memo; }
};

}

#endif
