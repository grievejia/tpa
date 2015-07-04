#include "CommandLineOptions.h"

#include "Dynamic/Instrument/MemoryInstrument.h"
#include "Transforms/RunPrepass.h"
#include "Util/IO/ReadIR.h"
#include "Util/IO/WriteIR.h"

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
		llvm::errs() << "Failed to read IR from " << opts.getInputFileName().data() << "\n";
		std::exit(-2);
	}

	// Run prepasses to canonicalize the IR
	if (!opts.isPrepassDisabled())
		transform::runPrepassOn(*module);

	// Instrument the module
	auto instrumenter = dynamic::MemoryInstrument();
	instrumenter.loadExternalTable(opts.getPtrConfigFileName().data());
	instrumenter.runOnModule(*module);

	// Output the instrumented program
	util::io::writeModuleToFile(*module, opts.getOutputFileName().data(), opts.isTextOutput());

	return 0;
}