#include "RunAnalysis.h"

#include "PointerAnalysis/FrontEnd/SemiSparseProgramBuilder.h"
#include "Util/IO/PointerAnalysis/WriteDotFile.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

void runAnalysisOnModule(const llvm::Module& module, const llvm::StringRef& dirName, bool writeFile)
{
	tpa::SemiSparseProgramBuilder builder;
	auto ssProg = builder.runOnModule(module);

	for (auto const& cfg: ssProg)
	{
		auto funName = cfg.getFunction().getName();
		llvm::outs() << "Processing PointerCFG for function " << funName << "...\n";

		if (writeFile)
		{
			auto fileName = dirName + funName + ".dot";
			llvm::outs() << "\tWriting output file " << fileName << "\n";
			util::io::writeDotFile(fileName.str().data(), cfg);
		}
	}
}