#include "PointerAnalysis/DataFlow/DefUseProgram.h"

using namespace llvm;

namespace tpa
{

bool DefUseGraphNode::insertTopLevelEdge(DefUseGraphNode* node)
{
	assert(node != nullptr);
	
	return topSucc.insert(node);
}

void DefUseGraphNode::insertMemLevelEdge(const MemoryLocation* loc, DefUseGraphNode* node)
{
	assert(loc != nullptr && node != nullptr);

	memSucc[loc].insert(node);
}

DefUseGraph* DefUseProgram::createDefUseGraphForFunction(const llvm::Function* f)
{
	assert(!dugMap.count(f) && "Inserting two DefUseGraph for the same function!");
	auto itr = dugMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(f),
		std::forward_as_tuple(f)
	).first;

	return &(itr->second);
}

const DefUseGraph* DefUseProgram::getDefUseGraph(const llvm::Function* f) const
{
	auto itr = dugMap.find(f);
	if (itr != dugMap.end())
		return &(itr->second);
	else 
		return nullptr;
}

DefUseGraph* DefUseProgram::getDefUseGraph(const llvm::Function* f)
{
	auto itr = dugMap.find(f);
	if (itr != dugMap.end())
		return &(itr->second);
	else 
		return nullptr;
}

}
