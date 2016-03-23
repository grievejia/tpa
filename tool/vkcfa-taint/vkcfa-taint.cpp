#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "Transforms/RunPrepass.h"
#include "Util/IO/ReadIR.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Signals.h>

using namespace llvm;

int main(int argc, char** argv)
{
	// Print full stack trace when crashed
	sys::PrintStackTraceOnErrorSignal();
	PrettyStackTraceProgram X(argc, argv);

	// Parse command line options
	auto opts = CommandLineOptions(argc, argv);

	// Read module from file
	auto module = util::io::readModuleFromFile(opts.getInputFileName().data());
	if (!module)
	{
		errs() << "Failed to read IR from " << opts.getInputFileName().data() << "\n";
		std::exit(-2);
	}

	// Run prepasses to canonicalize the IR
	if (!opts.isPrepassDisabled())
		transform::runPrepassOn(*module);

	// Run the analysis
	bool succ = runAnalysisOnModule(*module, opts);

	if (succ)
		outs() << "Congratulations! Taint check passed.\n";
	else
		outs() << "Taint check failed\n";

	return succ ? 0: -3;
}