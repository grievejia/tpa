#include "PointerControlFlow//PointerProgram.h"

#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
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
		os << "label=\"" << node->toString() << "\"]\n";
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
		os << "label=\"" << node->toString() << "\"]\n";
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
		auto fileName = dirName + "/" + prefix + "ptrdug." + mapping.first->getName() + ".dot";
		mapping.second.writeDefUseDotFile(fileName.str());
	}
}

void PointerCFG::writeDotFile(const StringRef& pathName) const
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

void PointerCFG::writeDefUseDotFile(const StringRef& pathName) const
{
	errs() << "Writing '" << pathName << "'...";

	std::string errorInfo;
	raw_fd_ostream file(pathName.data(), errorInfo, sys::fs::F_Text);

	if (errorInfo.empty())
		writeDefUseGraph(file, this);
	else
		errs() << "  error opening file for writing!";
	errs() << "\n";
}

std::string EntryNode::toString() const
{
	return "[ENTRY]";
}

std::string AllocNode::toString() const
{
	std::string ret;
	raw_string_ostream os(ret);
	os << "[ALLOC] " << getDest()->getName() << " :: ";
	auto allocType = getAllocType();
	if (allocType->isStructTy() && cast<StructType>(allocType)->hasName())
		os << allocType->getStructName();
	else
		os << *allocType;
	return os.str();
}

std::string ReturnNode::toString() const
{
	std::string rets;
	raw_string_ostream os(rets);
	os << "[RET] return ";
	if (ret != nullptr)
	{
		if (isa<ConstantPointerNull>(ret))
			os << "null";
		else
			os << ret->getName();
	}
	return os.str();
}

std::string CopyNode::toString() const
{
	std::string ret;
	raw_string_ostream os(ret);
	os << "[COPY] " << getDest()->getName() << " = ";
	if (offset > 0)
	{
		os << getFirstSrc()->getName() << " + " << offset;
		if (arrayRef)
			os << " []";
	}
	else
	{
		for (auto const& src: srcs)
			os << src->getName() << " ";
	}
	return os.str();
}

std::string LoadNode::toString() const
{
	std::string ret;
	raw_string_ostream os(ret);
	os << "[LOAD] " << getDest()->getName() << " = *" << src->getName();
	return os.str();
}

std::string StoreNode::toString() const
{
	std::string ret;
	raw_string_ostream os(ret);
	os << "[STORE] *" << dest->getName() << " = " << src->getName();
	return os.str();
}

std::string CallNode::toString() const
{
	std::string ret;
	raw_string_ostream os(ret);
	os << "[CALL] ";
	if (dest != nullptr)
		os << dest->getName() << " = ";

	os << funPtr->getName() << " ( ";
	for (auto const& arg: args)
		os << arg->getName() << " ";
	os << ")";

	return os.str();
}

}
