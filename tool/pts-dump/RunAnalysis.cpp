#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "Context/KLimitContext.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/SemiSparseProgramBuilder.h"
#include "Util/DataStructure/VectorSet.h"
#include "Util/Iterator/MapValueIterator.h"
#include "Util/IO/PointerAnalysis/Printer.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;
using namespace util::io;

static void dumpPtsSetForValue(const Value* value, const SemiSparsePointerAnalysis& ptrAnalysis)
{
	assert(value != nullptr);
	if (!value->getType()->isPointerTy())
		return;

	auto ptrs = ptrAnalysis.getPointerManager().getPointersWithValue(value);
	if (ptrs.empty())
	{
		errs() << "val = " << *value << "\n";
		assert((isa<PHINode>(value) || isa<IntToPtrInst>(value)) && "cannot find corresponding ptr for value?");
		return;
	}

	for (auto const& ptr: ptrs)
	{
		errs() << *ptr->getContext() << "::";
		dumpValue(errs(), *value);
		errs() << "  -->>  " << ptrAnalysis.getPtsSet(ptr) << "\n";
	}
}

static void dumpPtsSetInBasicBlock(const BasicBlock& bb, const SemiSparsePointerAnalysis& ptrAnalysis)
{
	for (auto const& inst: bb)
		dumpPtsSetForValue(&inst, ptrAnalysis);
}

static void dumpPtsSetInFunction(const Function& f, const SemiSparsePointerAnalysis& ptrAnalysis)
{
	for (auto const& arg: f.args())
		dumpPtsSetForValue(&arg, ptrAnalysis);
	for (auto const& bb: f)
		dumpPtsSetInBasicBlock(bb, ptrAnalysis);
}

static void dumpAll(const Module& module, const SemiSparsePointerAnalysis& ptrAnalysis)
{
	for (auto const& g: module.globals())
		dumpPtsSetForValue(&g, ptrAnalysis);
	for (auto const& f: module)
	{
		if (!f.isDeclaration())
			dumpPtsSetInFunction(f, ptrAnalysis);
	}
}

void runAnalysisOnModule(const Module& module, const CommandLineOptions& opts)
{
	SemiSparseProgramBuilder ssProgBuilder;
	auto ssProg = ssProgBuilder.runOnModule(module);
	auto ptrAnalysis = SemiSparsePointerAnalysis();
	ptrAnalysis.loadExternalPointerTable(opts.getPtrConfigFileName().data());

	context::KLimitContext::setLimit(opts.getContextSensitivity());
	ptrAnalysis.runOnProgram(ssProg);

	dumpAll(module, ptrAnalysis);
}