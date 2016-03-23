#include "PointerAnalysis/Program/SemiSparseProgram.h"

#include <llvm/IR/Module.h>

using namespace llvm;

namespace tpa
{

SemiSparseProgram::SemiSparseProgram(const llvm::Module& m): module(m)
{
	for (auto const& f: module)
	{
		if (f.hasAddressTaken())
			addrTakenFuncList.push_back(&f);
	}
}

CFG& SemiSparseProgram::getOrCreateCFGForFunction(const llvm::Function& f)
{
	auto itr = cfgMap.find(&f);
	if (itr == cfgMap.end())
		itr = cfgMap.emplace(&f, f).first;
	return itr->second;
}
const CFG* SemiSparseProgram::getCFGForFunction(const llvm::Function& f) const
{
	auto itr = cfgMap.find(&f);
	if (itr == cfgMap.end())
		return nullptr;
	else
		return &itr->second;
}
const CFG* SemiSparseProgram::getEntryCFG() const
{
	auto mainFunc = module.getFunction("main");
	assert(mainFunc != nullptr && "Cannot find main function!");
	return getCFGForFunction(*mainFunc);
}

}
