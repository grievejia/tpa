#include "PointerAnalysis/ControlFlow/SemiSparseProgramBuilder.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "TPA/Pass/CFGBuildingPass.h"

#include <llvm/IR/Module.h>

using namespace llvm;

namespace tpa
{

bool CFGBuildingPass::runOnModule(Module& module)
{
	auto builder = PointerProgramBuilder();

	auto prog = builder.buildPointerProgram(module);

	prog.writeDefUseDotFile("dots", "");

	return false;
}

void CFGBuildingPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

bool SemiSparseCFGBuildingPass::runOnModule(Module& module)
{
	auto builder = SemiSparseProgramBuilder();

	auto prog = builder.buildSemiSparseProgram(module);

	prog.writeDefUseDotFile("dots", "ss.");

	return false;
}

void SemiSparseCFGBuildingPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

char CFGBuildingPass::ID = 0;
static RegisterPass<CFGBuildingPass> X("build-cfg", "Build the pointer cfg", true, true);

char SemiSparseCFGBuildingPass::ID = 0;
static RegisterPass<SemiSparseCFGBuildingPass> Y("build-ss-cfg", "Build semi-sparse pointer cfg", true, true);

}
