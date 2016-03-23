#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "Transforms/RunPrepass.h"
#include "Util/IO/ReadIR.h"
#include "Util/IO/WriteIR.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

int main(int argc, char** argv)
{
	// Print full stack trace when crashed
	sys::PrintStackTraceOnErrorSignal();
	PrettyStackTraceProgram X(argc, argv);

	// Unsync iostream with C I/O libraries to accelerate standard iostreams
	std::ios::sync_with_stdio(false);

	// Parse command line options
	auto opts = CommandLineOptions(argc, argv);

	// Read module from file
	auto module = util::io::readModuleFromFile(opts.getInputFileName().data());
	if (!module)
	{
		errs() << "Failed to read IR from " << opts.getInputFileName() << "\n";
		std::exit(-2);
	}

	// Run prepasses to canonicalize the IR
	if (!opts.isPrepassDisabled())
		transform::runPrepassOn(*module);

	// Run the analysis
	runAnalysisOnModule(*module, opts);

	// Output the IR if told to do so
	if (!opts.getOutputFileName().empty())
		util::io::writeModuleToText(*module, opts.getOutputFileName().data());

	return 0;
}