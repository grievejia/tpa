#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "Context/KLimitContext.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/SemiSparseProgramBuilder.h"
#include "TaintAnalysis/Analysis/TrackingTaintAnalysis.h"
#include "TaintAnalysis/FrontEnd/DefUseModuleBuilder.h"
#include "Util/IO/TaintAnalysis/Printer.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace context;
using namespace llvm;
using namespace tpa;
using namespace taint;
using namespace util::io;

bool runAnalysisOnModule(const Module& module, const CommandLineOptions& opts)
{
	KLimitContext::setLimit(0);

	SemiSparseProgramBuilder ssProgBuilder;
	auto ssProg = ssProgBuilder.runOnModule(module);

	SemiSparsePointerAnalysis ptrAnalysis;
	ptrAnalysis.loadExternalPointerTable(opts.getPtrConfigFileName().data());
	ptrAnalysis.runOnProgram(ssProg);

	DefUseModuleBuilder builder(ptrAnalysis);
	builder.loadExternalModRefTable(opts.getModRefConfigFileName().data());
	auto duModule = builder.buildDefUseModule(module);

	TrackingTaintAnalysis taintAnalysis(ptrAnalysis);
	taintAnalysis.loadExternalTaintTable(opts.getTaintConfigFileName().data());
	auto ret = taintAnalysis.runOnDefUseModule(duModule);

	for (auto const& pp: ret.second)
		errs() << "Find loss site " << pp << "\n";

	return ret.first;
}