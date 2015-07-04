#pragma once

#include "PointerAnalysis/Program/SemiSparseProgram.h"

namespace llvm
{
	class Function;
	class Module;
}

namespace tpa
{

class TypeMap;

class SemiSparseProgramBuilder
{
private:
	void buildCFGForFunction(tpa::SemiSparseProgram&, const llvm::Function&, const TypeMap&);
public:
	tpa::SemiSparseProgram runOnModule(const llvm::Module& module);
};

}
