#pragma once

#include "Util/Hashing.h"

#include <llvm/IR/Instruction.h>

#include <unordered_set>

namespace context
{

class ProgramPoint;

// This class represents a particualr calling context, which is represented by a stack of callsites
class Context
{
private:
	// The call stack is implemented by a linked list
	const llvm::Instruction* callSite;
	const Context* predContext;
	size_t sz;

	static std::unordered_set<Context> ctxSet;

	Context(): callSite(nullptr), predContext(nullptr), sz(0) {}
	Context(const llvm::Instruction* c, const Context* p): callSite(c), predContext(p), sz(p == nullptr ? 1 : p->sz + 1) {}
public:
	const llvm::Instruction* getCallSite() const { return callSite; }
	size_t size() const { return sz; }
	bool isGlobalContext() const { return sz == 0; }

	bool operator==(const Context& other) const
	{
		return callSite == other.callSite && predContext == other.predContext;
	}
	bool operator!=(const Context& other) const
	{
		return !(*this == other);
	}

	static const Context* pushContext(const Context* ctx, const llvm::Instruction* inst);
	static const Context* pushContext(const ProgramPoint&);
	static const Context* popContext(const Context* ctx);
	static const Context* getGlobalContext();

	static std::vector<const Context*> getAllContexts();
	friend struct std::hash<Context>;
};

}

namespace std
{
	template<> struct hash<context::Context>
	{
		size_t operator()(const context::Context& c) const
		{
			return util::hashPair(c.callSite, c.predContext);
		}
	};
}
