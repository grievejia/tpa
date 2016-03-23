#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "Context/KLimitContext.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/SemiSparseProgramBuilder.h"
#include "TaintAnalysis/Analysis/TaintAnalysis.h"
#include "TaintAnalysis/FrontEnd/DefUseModuleBuilder.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace context;
using namespace llvm;
using namespace tpa;
using namespace taint;

bool runAnalysisOnModule(const Module& module, const CommandLineOptions& opts)
{
	KLimitContext::setLimit(opts.getContextSensitivity());

	SemiSparseProgramBuilder ssProgBuilder;
	auto ssProg = ssProgBuilder.runOnModule(module);

	SemiSparsePointerAnalysis ptrAnalysis;
	ptrAnalysis.loadExternalPointerTable(opts.getPtrConfigFileName().data());
	ptrAnalysis.runOnProgram(ssProg);

	DefUseModuleBuilder builder(ptrAnalysis);
	builder.loadExternalModRefTable(opts.getModRefConfigFileName().data());
	auto duModule = builder.buildDefUseModule(module);

	TaintAnalysis taintAnalysis(ptrAnalysis);
	taintAnalysis.loadExternalTaintTable(opts.getTaintConfigFileName().data());
	return taintAnalysis.runOnDefUseModule(duModule);
}