#include "PointerAnalysis/ControlFlow/PointerCFGNodePrint.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"

#include <llvm/Support/FileSystem.h>

using namespace llvm;

namespace tpa
{

static void writeGraph(raw_ostream& os, const PointerCFG* cfg)
{
	os << "digraph \"" << "PointerCFG for function " << cfg->getFunction()->getName() << "\" {\n";

	for (auto node: *cfg)
	{
		os << "\tNode" << node << " [shape=record,";
		os << "label=\"" << *node << "\"]\n";
		for (auto succ: node->succs())
			os << "\tNode" << node << " -> " << "Node" << succ << "\n";
	}
	os << "}\n";
}

static void writeDefUseGraph(raw_ostream& os, const PointerCFG* cfg)
{
	os << "digraph \"" << "PointerCFG with def-use edges for function " << cfg->getFunction()->getName() << "\" {\n";

	for (auto node: *cfg)
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

void PointerProgram::writeDotFile(const StringRef& dirName, const StringRef& prefix) const
{
	for (auto const& mapping: cfgMap)
	{
		auto fileName = dirName + "/" + prefix + "ptrcfg." + mapping.first->getName() + ".dot";
		mapping.second.writeDotFile(fileName.str());
	}
}

void PointerProgram::writeDefUseDotFile(const StringRef& dirName, const StringRef& prefix) const
{
	for (auto const& mapping: cfgMap)
	{
		auto fileName = dirName + "/" + prefix + "ptrcfg." + mapping.first->getName() + ".dot";
		mapping.second.writeDefUseDotFile(fileName.str());
	}
}

void PointerCFG::writeDotFile(const StringRef& pathName) const
{
	errs() << "Writing '" << pathName << "'...";

	std::error_code errorCode;
	raw_fd_ostream file(pathName.data(), errorCode, sys::fs::F_Text);

	if (!errorCode)
		writeGraph(file, this);
	else
		errs() << "  error opening file for writing!";
	errs() << "\n";
}

void PointerCFG::writeDefUseDotFile(const StringRef& pathName) const
{
	errs() << "Writing '" << pathName << "'...";

	std::error_code errorCode;
	raw_fd_ostream file(pathName.data(), errorCode, sys::fs::F_Text);

	if (!errorCode)
		writeDefUseGraph(file, this);
	else
		errs() << "  error opening file for writing!";
	errs() << "\n";
}

}
