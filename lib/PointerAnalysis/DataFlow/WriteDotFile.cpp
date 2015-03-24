#include "MemoryModel/Memory/Memory.h"
#include "PointerAnalysis/DataFlow/DefUseGraphNodePrint.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "PointerAnalysis/DataFlow/DefUseProgram.h"

#include <llvm/Support/FileSystem.h>

#include <unordered_map>

using namespace llvm;

namespace tpa
{

namespace
{

void writeGraph(raw_ostream& os, const DefUseFunction& duFunc)
{
	os << "digraph \"" << "DefUseGraph for function " << duFunc.getFunction().getName() << "\" {\n";

	using MemSet = VectorSet<const MemoryLocation*>;

	auto dumpInst = [&os] (const DefUseInstruction* duInst)
	{
		os << "\tNode" << duInst << " [shape=record,";
		if (auto inst = duInst->getInstruction())
			os << "label=\"" << *inst << "\"]\n";
		else
			os << "label=\"Entry\"]\n";
		
		for (auto succ: duInst->top_succs())
			os << "\tNode" << duInst << " -> " << "Node" << succ << " [style=dotted]\n";

		std::unordered_map<const DefUseInstruction*, MemSet> succMap;
		for (auto const& mapping: duInst->mem_succs())
			for (auto succ: mapping.second)
				succMap[succ].insert(mapping.first);

		for (auto const& mapping: succMap)
		{
			os << "\tNode" << duInst << " -> " << "Node" << mapping.first << " [label=\"";
			for (auto loc: mapping.second)
				os << *loc << "\\n";
			os << "\"]\n";
		}
	};

	dumpInst(duFunc.getEntryInst());
	for (auto const& bb: duFunc.getFunction())
	{
		for (auto& inst: bb)
		{
			auto duInst = duFunc.getDefUseInstruction(&inst);
			dumpInst(duInst);
		}
	}

	os << "}\n";
}

void writeGraph(raw_ostream& os, const DefUseGraph& dug)
{
	os << "digraph \"" << "DefUseGraph for function " << dug.getFunction()->getName() << "\" {\n";

	using MemSet = VectorSet<const MemoryLocation*>;

	for (auto node: dug)
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

template <typename GraphType>
void writeDotFile(const StringRef& pathName, const GraphType& graph)
{
	errs() << "Writing '" << pathName << "'...";

	std::error_code errorCode;
	raw_fd_ostream file(pathName.data(), errorCode, sys::fs::F_Text);

	if (!errorCode)
		writeGraph(file, graph);
	else
		errs() << "  error opening file for writing!";
	errs() << "\n";
}

}	// end of anonymous namespace

void DefUseGraph::writeDotFile(const StringRef& pathName) const
{
	::tpa::writeDotFile(pathName, *this);
}

void DefUseFunction::writeDotFile(const StringRef& pathName) const
{
	::tpa::writeDotFile(pathName, *this);
}

void DefUseProgram::writeDotFile(const StringRef& dirName, const StringRef& prefix) const
{
	for (auto const& mapping: dugMap)
	{
		auto fileName = dirName + "/" + prefix + "ptrdug." + mapping.first->getName() + ".dot";
		mapping.second.writeDotFile(fileName.str());
	}
}

void DefUseModule::writeDotFile(const StringRef& dirName, const StringRef& prefix) const
{
	for (auto const& mapping: funMap)
	{
		auto fileName = dirName + "/" + prefix + "." + mapping.first->getName() + ".dot";
		mapping.second.writeDotFile(fileName.str());
	}
}

}
