#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "Context/KLimitContext.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/SemiSparseProgramBuilder.h"
#include "TaintAnalysis/FrontEnd/DefUseModuleBuilder.h"
#include "Util/IO/TaintAnalysis/WriteDotFile.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace taint;
using namespace tpa;

void runAnalysisOnModule(const Module& module, const CommandLineOptions& opts)
{
	SemiSparseProgramBuilder ssBuilder;
	auto ssProg = ssBuilder.runOnModule(module);

	context::KLimitContext::setLimit(opts.getContextSensitivity());
	SemiSparsePointerAnalysis ptrAnalysis;
	ptrAnalysis.loadExternalPointerTable(opts.getPtrConfigFileName().data());
	ptrAnalysis.runOnProgram(ssProg);

	DefUseModuleBuilder builder(ptrAnalysis);
	builder.loadExternalModRefTable(opts.getModRefConfigFileName().data());
	auto duModule = builder.buildDefUseModule(module);

	for (auto const& duFunc: duModule)
	{
		auto funName = duFunc.getFunction().getName();
		llvm::outs() << "Processing DefUseModule for function " << funName << "...\n";

		if (!opts.isDryRun())
		{
			auto fileName = opts.getOutputDirName() + funName + ".du_mod.dot";
			llvm::outs() << "\tWriting output file " << fileName << "\n";
			util::io::writeDotFile(fileName.str().data(), duFunc);
		}
	}
}