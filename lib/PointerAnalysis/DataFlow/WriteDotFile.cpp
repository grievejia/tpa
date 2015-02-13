#include "MemoryModel/Memory/Memory.h"
#include "PointerAnalysis/DataFlow/DefUseGraphNodePrint.h"
#include "PointerAnalysis/DataFlow/DefUseProgram.h"

#include <llvm/Support/FileSystem.h>

#include <unordered_map>

using namespace llvm;

namespace tpa
{

static void writeGraph(raw_ostream& os, const DefUseGraph* dug)
{
	os << "digraph \"" << "DefUseGraph for function " << dug->getFunction()->getName() << "\" {\n";

	using MemSet = VectorSet<const MemoryLocation*>;


	for (auto node: *dug)
	{
		os << "\tNode" << node << " [shape=record,";
		os << "label=\"" << *node << "\"]\n";
		for (auto succ: node->top_succs())
			os << "\tNode" << node << " -> " << "Node" << succ << " [style=dotted]\n";

		std::unordered_map<const DefUseGraphNode*, MemSet> succMap;
		for (auto const& mapping: node->mem_succs())
			for (auto succ: mapping.second)
				succMap[succ].insert(mapping.first);

		for (auto const& mapping: succMap)
		{
			os << "\tNode" << node << " -> " << "Node" << mapping.first << " [label=\"";
			for (auto loc: mapping.second)
				os << *loc << "\\n";
			os << "\"]\n";
		}
	}
	os << "}\n";
}

void DefUseGraph::writeDotFile(const StringRef& pathName) const
{
	errs() << "Writing '" << pathName << "'...";

	std::string errorInfo;
	raw_fd_ostream file(pathName.data(), errorInfo, sys::fs::F_Text);

	if (errorInfo.empty())
		writeGraph(file, this);
	else
		errs() << "  error opening file for writing!";
	errs() << "\n";
}

void DefUseProgram::writeDotFile(const StringRef& dirName, const StringRef& prefix) const
{
	for (auto const& mapping: dugMap)
	{
		auto fileName = dirName + "/" + prefix + "ptrdug." + mapping.first->getName() + ".dot";
		mapping.second.writeDotFile(fileName.str());
	}
}

}
