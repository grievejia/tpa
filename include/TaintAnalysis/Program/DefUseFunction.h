#pragma once

#include "TaintAnalysis/Program/DefUseInstruction.h"

namespace taint
{

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
};

}
