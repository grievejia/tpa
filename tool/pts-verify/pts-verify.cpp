#include "CommandLineOptions.h"
#include "RunAnalysis.h"

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
		errs() << "Failed to read IR from " << opts.getInputFileName() << "\n";
		std::exit(-2);
	}

	// Run the analysis
	bool succ = runAnalysisOnModule(*module, opts.getInputLogName().data(), opts.getPtrConfigFileName().data(), opts.getContextSensitivity());

	if (succ)
		outs() << "Congratulations! All tests passed.\n";

	return succ ? 0: -3;
}