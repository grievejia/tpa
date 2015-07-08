#include "TaintAnalysis/Program/DefUseModule.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace llvm;

namespace taint
{

const Function* DefUseInstruction::getFunction() const
{
	assert(inst != nullptr);
	return inst->getParent()->getParent();
}

bool DefUseInstruction::isEntryInstruction() const
{
	return (inst == nullptr);
}

bool DefUseInstruction::isCallInstruction() const
{
	return (inst != nullptr) && (llvm::isa<CallInst>(inst) || llvm::isa<InvokeInst>(inst));
}

bool DefUseInstruction::isReturnInstruction() const
{
	return (inst != nullptr) && (llvm::isa<ReturnInst>(inst));
}

DefUseInstruction* DefUseFunction::getDefUseInstruction(const Instruction* inst)
{
	auto itr = instMap.find(inst);
	assert(itr != instMap.end());
	return &itr->second;
}
const DefUseInstruction* DefUseFunction::getDefUseInstruction(const Instruction* inst) const
{
	auto itr = instMap.find(inst);
	assert(itr != instMap.end());
	return &itr->second;
}

DefUseFunction::DefUseFunction(const Function& f): function(f), exitInst(nullptr)
{
	for (auto const& bb: f)
	{
		for (auto const& inst: bb)
		{
			auto itr = instMap.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(&inst),
				std::forward_as_tuple(inst)
			).first;

			if (isa<ReturnInst>(&inst))
			{
				if (exitInst == nullptr)
					exitInst = &itr->second;
				else
					llvm_unreachable("Multiple return inst detected!");
			}
		}
	}
}

DefUseModule::DefUseModule(const Module& m): module(m), entryFunc(nullptr)
{
	for (auto const& f: module)
	{
		if (f.isDeclaration())
			continue;

		auto itr = funMap.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(&f),
			std::forward_as_tuple(f)
		).first;

		if (f.getName() == "main")
		{
			assert(entryFunc == nullptr);
			entryFunc = &itr->second;
		}
	}
}

const DefUseFunction& DefUseModule::getDefUseFunction(const Function* f) const
{
	auto itr = funMap.find(f);
	assert(itr != funMap.end());
	return itr->second;
}

}
