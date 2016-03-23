#include "PointerAnalysis/Program/CFG/CFG.h"
#include "Util/IO/PointerAnalysis/Printer.h"
#include "Util/IO/PointerAnalysis/WriteDotFile.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ToolOutputFile.h>

using namespace llvm;

namespace util
{
namespace io
{

static void writeCFG(raw_ostream& os, const tpa::CFG& cfg)
{
	os << "digraph \"" << "PointerCFG with def-use edges for function " << cfg.getFunction().getName() << "\" {\n";

	for (auto node: cfg)
	{
		os << "\tNode" << node << " [shape=record,";
		os << "label=\"" << *node << "\"]\n";
		for (auto succ: node->succs())
			os << "\tNode" << node << " -> " << "Node" << succ << "\n";
		for (auto succ: node->uses())
			os << "\tNode" << node << " -> " << "Node" << succ << " [style=dotted]\n";
	}
	os << "}\n";
}

void writeDotFile(const char* filePath, const tpa::CFG& cfg)
{
	std::error_code ec;
	tool_output_file out(filePath, ec, sys::fs::F_None);
	if (ec)
	{
		errs() << "Failed to open file " << filePath << ": " << ec.message() << "\n";
		return;
	}

	writeCFG(out.os(), cfg);

	out.keep();
}

}
}