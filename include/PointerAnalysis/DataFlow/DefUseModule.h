#ifndef TPA_DEF_USE_MODULE_H
#define TPA_DEF_USE_MODULE_H

#include "Utils/VectorSet.h"

#include <llvm/ADT/iterator_range.h>

#include <cassert>
#include <limits>
#include <unordered_map>

namespace llvm
{
	class Function;
	class Instruction;
	class Module;
	class StringRef;
}

namespace tpa
{

class MemoryLocation;

class DefUseInstruction
{
private:
	const llvm::Instruction* inst;
	// The reverse postorder number of this node
	size_t rpo;

	using NodeSet = VectorSet<DefUseInstruction*>;
	NodeSet topSucc, topPred;

	using NodeMap = std::unordered_map<const MemoryLocation*, NodeSet>;
	NodeMap memSucc, memPred;

	DefUseInstruction(): inst(nullptr), rpo(std::numeric_limits<size_t>::max()) {}
public:
	using iterator = NodeSet::iterator;
	using const_iterator = NodeSet::const_iterator;
	using mem_succ_iterator = NodeMap::iterator;
	using const_mem_succ_iterator = NodeMap::const_iterator;

	DefUseInstruction(const llvm::Instruction& i): inst(&i), rpo(0) {}
	const llvm::Instruction* getInstruction() const { return inst; }

	const llvm::Function* getFunction() const;
	bool isEntryInstruction() const;
	bool isCallInstruction() const;
	bool isReturnInstruction() const;

	size_t getPriority() const
	{
		assert(rpo != 0);
		return rpo;
	}
	bool hasPriority() const { return rpo != 0; }
	void setPriority(size_t o)
	{
		assert(o != 0 && "0 cannot be used as a priority num!");
		rpo = o;
	}

	void insertTopLevelEdge(DefUseInstruction* node)
	{
		assert(node != nullptr);
		topSucc.insert(node);
		node->topPred.insert(this);
	}
	void insertMemLevelEdge(const MemoryLocation* loc, DefUseInstruction* node)
	{
		assert(loc != nullptr && node != nullptr);
		memSucc[loc].insert(node);
		node->memPred[loc].insert(this);
	}

	llvm::iterator_range<iterator> top_succs()
	{
		return llvm::iterator_range<iterator>(topSucc.begin(), topSucc.end());
	}
	llvm::iterator_range<const_iterator> top_succs() const
	{
		return llvm::iterator_range<const_iterator>(topSucc.begin(), topSucc.end());
	}
	llvm::iterator_range<mem_succ_iterator> mem_succs()
	{
		return llvm::iterator_range<mem_succ_iterator>(memSucc.begin(), memSucc.end());
	}
	llvm::iterator_range<const_mem_succ_iterator> mem_succs() const
	{
		return llvm::iterator_range<const_mem_succ_iterator>(memSucc.begin(), memSucc.end());
	}

	llvm::iterator_range<iterator> top_preds()
	{
		return llvm::iterator_range<iterator>(topPred.begin(), topPred.end());
	}
	llvm::iterator_range<const_iterator> top_preds() const
	{
		return llvm::iterator_range<const_iterator>(topPred.begin(), topPred.end());
	}
	llvm::iterator_range<mem_succ_iterator> mem_preds()
	{
		return llvm::iterator_range<mem_succ_iterator>(memPred.begin(), memPred.end());
	}
	llvm::iterator_range<const_mem_succ_iterator> mem_preds() const
	{
		return llvm::iterator_range<const_mem_succ_iterator>(memPred.begin(), memPred.end());
	}

	friend class DefUseFunction;
};

class DefUseFunction
{
private:
	const llvm::Function& function;
	std::unordered_map<const llvm::Instruction*, DefUseInstruction> instMap;

	DefUseInstruction entryInst;
	DefUseInstruction* exitInst;
public:
	using NodeType = DefUseInstruction;

	DefUseFunction(const llvm::Function& f);
	DefUseFunction(const DefUseFunction&) = delete;
	DefUseFunction& operator=(const DefUseFunction&) = delete;
	const llvm::Function& getFunction() const { return function; }

	DefUseInstruction* getDefUseInstruction(const llvm::Instruction*);
	const DefUseInstruction* getDefUseInstruction(const llvm::Instruction*) const;	

	DefUseInstruction* getEntryInst()
	{
		return &entryInst;
	}
	const DefUseInstruction* getEntryInst() const
	{
		return &entryInst;
	}
	DefUseInstruction* getExitInst()
	{
		return exitInst;
	}
	const DefUseInstruction* getExitInst() const
	{
		return exitInst;
	}

	void writeDotFile(const llvm::StringRef& filePath) const;
};

class DefUseModule
{
private:
	const llvm::Module& module;
	using FunctionMap = std::unordered_map<const llvm::Function*, DefUseFunction>;
	FunctionMap funMap;
	const DefUseFunction* entryFunc;
public:
	using iterator = FunctionMap::iterator;
	using const_iterator = FunctionMap::const_iterator;

	DefUseModule(const llvm::Module& m);
	const llvm::Module& getModule() const { return module; }

	const DefUseFunction& getEntryFunction() const
	{
		assert(entryFunc != nullptr);
		return *entryFunc;
	};
	const DefUseFunction& getDefUseFunction(const llvm::Function* f) const;

	void writeDotFile(const llvm::StringRef& dirName, const llvm::StringRef& prefix) const;

	iterator begin() { return funMap.begin(); }
	iterator end() { return funMap.end(); }
	const_iterator begin() const { return funMap.begin(); }
	const_iterator end() const { return funMap.end(); }
};

}

#endif
