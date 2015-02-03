#include "ControlFlow/PointerProgram.h"

#include <llvm/IR/Instructions.h>

using namespace llvm;

namespace tpa
{

void PointerCFGNode::insertEdge(PointerCFGNode* node)
{
	assert(node != nullptr);
	
	succ.insert(node);
	(node->pred).insert(this);
}

void PointerCFGNode::removeEdge(PointerCFGNode* node)
{
	assert(node != nullptr);

	succ.erase(node);
	(node->pred).erase(this);
}

void PointerCFGNode::insertDefUseEdge(PointerCFGNode* node)
{
	assert(node != nullptr);

	use.insert(node);
	node->def.insert(this);
}

void PointerCFGNode::removeDefUseEdge(PointerCFGNode* node)
{
	assert(node != nullptr);

	use.erase(node);
	node->def.erase(this);
}

Type* AllocNode::getAllocType() const
{
	return cast<AllocaInst>(getInstruction())->getAllocatedType();
}

PointerCFG::PointerCFG(const Function* f): function(f), entryNode(nullptr), exitNode(nullptr)
{
	assert(f != nullptr);
	entryNode = new EntryNode();
	nodes.emplace_back(entryNode);
}

PointerCFG* PointerProgram::createPointerCFGForFunction(const llvm::Function* f)
{
	assert(!cfgMap.count(f) && "Inserting two PointerCFG for the same function!");
	auto itr = cfgMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(f),
		std::forward_as_tuple(f)
	).first;
	
	auto cfg = &(itr->second);
	if (f->getName() == "main")
	{
		assert(entryCFG == nullptr && "Two main() function in a single module?");
		entryCFG = cfg;
	}

	if (f->hasAddressTaken())
		addrTakenFunctions.push_back(f);

	return cfg;
}

const PointerCFG* PointerProgram::getPointerCFG(const llvm::Function* f) const
{
	auto itr = cfgMap.find(f);
	if (itr != cfgMap.end())
		return &(itr->second);
	else 
		return nullptr;
}

PointerCFG* PointerProgram::getPointerCFG(const llvm::Function* f)
{
	auto itr = cfgMap.find(f);
	if (itr != cfgMap.end())
		return &(itr->second);
	else 
		return nullptr;
}

}
