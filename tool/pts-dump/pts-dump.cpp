#include "PtsDumpPass.h"
#include "Transforms/ExpandByVal.h"
#include "Transforms/ExpandConstantExpr.h"
#include "Transforms/ExpandGetElementPtr.h"
#include "Transforms/ExpandIndirectBr.h"
#include "Transforms/GlobalCleanup.h"

#include <llvm/ADT/Triple.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

using namespace llvm;

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bitcode file>"),
    cl::init("-"), cl::value_desc("filename"));

static cl::opt<bool> PrintIROnly("ir-only", cl::desc("Print the transformed IR instead of the points-to sets"), cl::init(false));

void initializeLLVMPasses()
{
	PassRegistry &Registry = *PassRegistry::getPassRegistry();
	initializeCore(Registry);
	initializeScalarOpts(Registry);
	initializeIPO(Registry);
	initializeTransformUtils(Registry);
}

std::unique_ptr<Module> loadInputModule(const std::string& fileName)
{
	// Load the input module...
	LLVMContext &context = getGlobalContext();
	SMDiagnostic err;
	return std::move(parseIRFile(fileName, err, context));
}

legacy::PassManager getAllPasses(const Module& module)
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
	passes.add(createGlobalOptimizerPass());
	passes.add(createInstructionCombiningPass());
	passes.add(new ExpandConstantExprPass());
	passes.add(new ExpandGetElementPtrPass());
	passes.add(createGlobalDCEPass());
	passes.add(createAggressiveDCEPass());
	passes.add(createLoopDeletionPass());
	passes.add(createDeadArgEliminationPass());
	passes.add(createCFGSimplificationPass());
	passes.add(createUnifyFunctionExitNodesPass());
	passes.add(createInstructionNamerPass());
	if (!PrintIROnly)
		passes.add(new PtsDumpPass());

	return passes;
}

int main(int argc, char** argv)
{
	// Print full stack trace when crashed
	sys::PrintStackTraceOnErrorSignal();
	PrettyStackTraceProgram X(argc, argv);

	// Parsing command line
	cl::ParseCommandLineOptions(argc, argv, "Tunable Pointer Analysis\n");

	// Initialize LLVM passes
	initializeLLVMPasses();

	// Read module from file
	auto module = loadInputModule(InputFilename);
	if (!module)
	{
		errs() << "Failed to read IR from " << InputFilename << "\n";
		return -1;
	}

	// Run designated passes
	auto passManager = getAllPasses(*module);
	passManager.run(*module);

	if (PrintIROnly)
		outs() << *module << "\n";

	return 0;
}