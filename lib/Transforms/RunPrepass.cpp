#include "Transforms/ExpandByVal.h"
#include "Transforms/ExpandConstantExpr.h"
#include "Transforms/ExpandGetElementPtr.h"
#include "Transforms/ExpandIndirectBr.h"
#include "Transforms/GlobalCleanup.h"
#include "Transforms/RunPrepass.h"

#include <llvm/ADT/Triple.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

using namespace llvm;

namespace transform
{

static void initializePrepasses()
{
	PassRegistry &Registry = *PassRegistry::getPassRegistry();
	initializeCore(Registry);
	initializeScalarOpts(Registry);
	initializeIPO(Registry);
	initializeTransformUtils(Registry);
}

static void runAllPrepasses(Module& module)
{
	legacy::PassManager passes;

	passes.add(new TargetLibraryInfo(Triple(module.getTargetTriple())));
	passes.add(new DataLayoutPass());

	passes.add(createLowerAtomicPass());
	passes.add(createLowerInvokePass());
	passes.add(createLowerSwitchPass());
	passes.add(new ResolveAliases());
	passes.add(new ExpandIndirectBr());
	passes.add(new ExpandByValPass());
	passes.add(createInstructionCombiningPass());
	passes.add(createIPConstantPropagationPass());
	passes.add(createDeadArgEliminationPass());
	passes.add(createConstantPropagationPass());
	passes.add(createCFGSimplificationPass());
	passes.add(createGlobalDCEPass());
	passes.add(createAggressiveDCEPass());
	passes.add(createLoopDeletionPass());
	passes.add(createGlobalOptimizerPass());
	passes.add(createCFGSimplificationPass());
	passes.add(createUnifyFunctionExitNodesPass());
	passes.add(new ExpandConstantExprPass());
	passes.add(new ExpandGetElementPtrPass());
	passes.add(createInstructionNamerPass());

	passes.run(module);
}

void runPrepassOn(Module& module)
{
	initializePrepasses();
	runAllPrepasses(module);
}

}
