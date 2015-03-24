#include "PointerAnalysis/ControlFlow/PointerProgramBuilder.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_set>

using namespace llvm;

namespace tpa
{

void PointerProgramBuilder::computeNodePriority(PointerCFG& cfg)
{
	size_t currPrio = 1;
	for (auto itr = po_begin(&cfg), ite = po_end(&cfg); itr != ite; ++itr)
		itr->setPriority(currPrio++);

	// po_iterator doesn't seem to touch the orphan nodes
	for (auto node: cfg)
		if (!node->hasPriority())
			node->setPriority(currPrio++);
}

PointerCFGNode* PointerProgramBuilder::translateInstruction(PointerCFG& cfg, const llvm::Instruction& inst)
{
	bool isPtr = inst.getType()->isPointerTy();

	switch (inst.getOpcode())
	{
		case Instruction::Alloca:
		{
			assert(isPtr && "Alloca instruction should have a pointer type");

			return cfg.create<AllocNode>(&inst);
		}
		case Instruction::Load:
		{
			// We are not interested in scalar loads
			if (!isPtr)
				return nullptr;

			auto loadInst = cast<LoadInst>(&inst);

			return cfg.create<LoadNode>(loadInst, loadInst->getPointerOperand()->stripPointerCasts());
		}
		case Instruction::Store:
		{
			auto storeInst = cast<StoreInst>(&inst);

			auto dst = storeInst->getPointerOperand()->stripPointerCasts();
			auto src = storeInst->getValueOperand()->stripPointerCasts();

			// We are not interested in scalar stores
			if (!src->getType()->isPointerTy())
				return nullptr;

			return cfg.create<StoreNode>(storeInst, dst, src);
		}
		case Instruction::Ret:
		{
			auto retInst = cast<ReturnInst>(&inst);

			auto retInstValue = retInst->getReturnValue();
			const Value* retValue = nullptr;
			if (retInstValue != nullptr && retInstValue->getType()->isPointerTy())
				retValue = retInstValue->stripPointerCasts();
			auto retNode = cfg.create<ReturnNode>(retInst, retValue);
			cfg.setExitNode(retNode);
			return retNode;
		}
		case Instruction::Invoke:
		case Instruction::Call:
		{
			ImmutableCallSite cs(&inst);

			auto funPtr = cs.getCalledValue()->stripPointerCasts();

			// Here we check if the callee is an external function that has no effect on pointers. If so, avoid creating node for that call
			auto callee = cs.getCalledFunction();
			if (callee == nullptr)
			{
				// The external function may get masked by bitcasts
				if (auto calledFunction = dyn_cast<Function>(funPtr))
					callee = calledFunction;
			}
			if (callee != nullptr)
			{
				if (callee->isDeclaration())
				{
					auto extType = extTable.lookup(callee->getName());
					if (extType == PointerEffect::UnknownEffect)
					{
						errs() << "Unrecognized external funciton call: " << callee->getName() << "\n";
						llvm_unreachable("");
					}
					// Removing NoEffect external calls has some negative impact on the client. Disable it for now.
					//else if (extType == PointerEffect::NoEffect)
					//	return nullptr;
				}
			}

			const Value* retVar = nullptr;
			if (isPtr)
				retVar = &inst;

			auto callNode = cfg.create<CallNode>(&inst, funPtr, retVar);

			for (unsigned i = 0; i < cs.arg_size(); ++i)
			{
				auto argVal = cs.getArgument(i)->stripPointerCasts();

				if (!argVal->getType()->isPointerTy())
					continue;
				if (isa<UndefValue>(argVal))
					continue;

				callNode->addArgument(argVal);
			}

	    	return callNode;
	    }
		case Instruction::PHI:
		{
			if (!isPtr)
				return nullptr;

			auto phiInst = cast<PHINode>(&inst);

			auto srcs = SmallPtrSet<const Value*, 4>();
			for (unsigned i = 0; i < phiInst->getNumIncomingValues(); ++i)
			{
				auto value = phiInst->getIncomingValue(i)->stripPointerCasts();
				if (isa<UndefValue>(value))
					continue;
				srcs.insert(value);
			}
			
			auto copyNode = cfg.create<CopyNode>(phiInst);
			for (auto src: srcs)
				copyNode->addSrc(src);

			return copyNode;
		}
		case Instruction::Select:
		{
			if (!isPtr)
				return nullptr;
			// Select instruction will be directly translated into copy node
			auto selectInst = cast<SelectInst>(&inst);

			auto src0 = selectInst->getFalseValue()->stripPointerCasts();
			auto src1 = selectInst->getTrueValue()->stripPointerCasts();

			auto copyNode = cfg.create<CopyNode>(selectInst);

			copyNode->addSrc(src0);
			if (src0 != src1)
				copyNode->addSrc(src1);
			
			return copyNode;
		}
		case Instruction::IntToPtr:
		{
			// We need to resolve some pointer arithmetics here: ptrtoint+inttoptr has the same effect as gep
			assert(isPtr);

			auto op = inst.getOperand(0);

			// Pointer copy: Y = inttoptr (ptrtoint X)
			Value* src = nullptr;
			if (PatternMatch::match(op, PatternMatch::m_PtrToInt(PatternMatch::m_Value(src))))
			{
				auto copyNode = cfg.create<CopyNode>(&inst);
				copyNode->addSrc(src);
				return copyNode;
			}
			
			Value* offsetValue = nullptr;
			if (PatternMatch::match(op,
				PatternMatch::m_Add(
					PatternMatch::m_PtrToInt(
						PatternMatch::m_Value(src)),
					PatternMatch::m_Value(offsetValue))))
			{
				ConstantInt* ci = nullptr;
				// Struct pointer arithmetic: Y = inttoptr (ptrtoint (X) + offset)
				// Constant-size array pointer arithmetic can produce the same pattern
				if (PatternMatch::match(offsetValue, PatternMatch::m_ConstantInt(ci)))
					return cfg.create<CopyNode>(&inst, src, ci->getSExtValue(), false);
				// Array pointer arithmetic: Y = inttoptr (ptrtoint (X) + somethingelse)
				else
				{
					auto cInt = dataLayout->getPointerSize();
					if (PatternMatch::match(offsetValue,
						PatternMatch::m_Mul(
							PatternMatch::m_Value(),
							PatternMatch::m_ConstantInt(ci))))
					{
						auto sExtVal = ci->getSExtValue();
						if (sExtVal > 0)
							cInt = sExtVal;
					}
					
					// Create a CopyNode that is aware that we are doing an array reference
					return cfg.create<CopyNode>(&inst, src, cInt, true);
				}
			}

			// In other cases, we make no effort to track what the rhs is. 
			errs() << "inst = " << inst << "\n";
			llvm_unreachable("Input program fabricates pointers from integer");
		}
		case Instruction::GetElementPtr:
		{
			assert(isPtr);

			auto gepInst = cast<GetElementPtrInst>(&inst);
			auto src = gepInst->getPointerOperand()->stripPointerCasts();

			// Constant-offset GEP
			auto gepOffset = APInt(dataLayout->getPointerTypeSizeInBits(src->getType()), 0);
			if (gepInst->accumulateConstantOffset(*dataLayout, gepOffset))
			{
				auto offset = gepOffset.getSExtValue();

				return cfg.create<CopyNode>(gepInst, src, offset, false);
			}

			// Variable-offset GEP
			auto numOps = gepInst->getNumOperands();
			if (numOps != 2 && numOps != 3)
				llvm_unreachable("Found a non-canonicalized GEP. Please run -expand-gep pass first!");

			auto offset = dataLayout->getPointerSize();
			if (numOps == 2)
				offset = dataLayout->getTypeAllocSize(cast<SequentialType>(src->getType())->getElementType());
			else
			{
				assert(isa<ConstantInt>(gepInst->getOperand(1)) && cast<ConstantInt>(gepInst->getOperand(1))->isZero());
				auto elemType = cast<SequentialType>(gepInst->getPointerOperand()->getType())->getElementType();
				offset = dataLayout->getTypeAllocSize(cast<SequentialType>(elemType)->getElementType());
			}

			return cfg.create<CopyNode>(gepInst, src, offset, true);
		}
		case Instruction::VAArg:
		{
			// We should not see those instructions here because they should have been removed by prepasses
			llvm_unreachable("vararg instruction not supported!");
			return nullptr;
		}
		case Instruction::ExtractValue:
		case Instruction::InsertValue:
		{
			if (!isPtr)
				return nullptr;
			llvm_unreachable("ev/iv instruction not supported!");
		}
		// Fall through
		case Instruction::LandingPad:
		case Instruction::Resume:
		case Instruction::AtomicRMW:
		case Instruction::AtomicCmpXchg:
		{
			errs() << inst << "\n";
			llvm_unreachable("We have no intention to support those instructions");
			return nullptr;
		}
		//No other ops should affect pointer values.
		default:
			// Do nothing
			return nullptr;
	}
	return nullptr;
}

void PointerProgramBuilder::buildPointerCFG(PointerCFG& cfg)
{
	auto bbToNode = DenseMap<const BasicBlock*, std::pair<PointerCFGNode*, PointerCFGNode*>>();
	auto nonEmptySuccMap = DenseMap<const BasicBlock*, std::vector<PointerCFGNode*>>();

	// Scan the basic blocks and create the nodes first. We will worry about how to connect them later
	for (auto const& currBlock: *cfg.getFunction())
	{
		PointerCFGNode* startNode = nullptr;
		PointerCFGNode* endNode = nullptr;

		for (auto const& inst: currBlock)
		{
			auto currNode = translateInstruction(cfg, inst);
			if (currNode == nullptr)
				continue;

			// Update the first node
			if (startNode == nullptr)
				startNode = currNode;
			// Chain the node with the last one
			if (endNode != nullptr)
				endNode->insertEdge(currNode);
			endNode = currNode;
		}

		assert((startNode == nullptr) == (endNode == nullptr));
		if (startNode != nullptr)
			bbToNode.insert(std::make_pair(&currBlock, std::make_pair(startNode, endNode)));
		else
			nonEmptySuccMap[&currBlock] = std::vector<PointerCFGNode*>();
	}

	// Now the biggest problem is those "empty blocks" (i.e. blocks that do not contain any PointerCFGNode). Those blocks may form cycles. So we need to know, in advance, what are the non empty successors of the empty blocks.
	auto processedEmptyBlock = std::unordered_set<const BasicBlock*>();
	for (auto& mapping: nonEmptySuccMap)
	{
		auto currBlock = mapping.first;
		auto succs = std::unordered_set<PointerCFGNode*>();

		// The following codes try to implement a non-recursive DFS for finding all reachable non-empty blocks starting from an empty block
		auto workList = std::vector<const BasicBlock*>();
		workList.insert(workList.end(), succ_begin(currBlock), succ_end(currBlock));
		auto visitedEmptyBlock = std::unordered_set<const BasicBlock*>();
		visitedEmptyBlock.insert(currBlock);

		while (!workList.empty())
		{
			auto nextBlock = workList.back();
			workList.pop_back();

			if (bbToNode.count(nextBlock))
			{
				succs.insert(bbToNode[nextBlock].first);
			}
			else if (processedEmptyBlock.count(nextBlock))
			{
				auto& nextVec = nonEmptySuccMap[nextBlock];
				succs.insert(nextVec.begin(), nextVec.end());
			}
			else
			{
				for (auto itr = succ_begin(nextBlock), ite = succ_end(nextBlock); itr != ite; ++itr)
				{
					auto nextNextBlock = *itr;
					if (visitedEmptyBlock.count(nextNextBlock))
						continue;
					if (!bbToNode.count(nextNextBlock))
						visitedEmptyBlock.insert(nextNextBlock);
					workList.push_back(nextNextBlock);
				}
			}
		}

		processedEmptyBlock.insert(currBlock);
		mapping.second.insert(mapping.second.end(), succs.begin(), succs.end());
	}
	
	for (auto& mapping: bbToNode)
	{
		auto bb = mapping.first;
		auto lastNode = mapping.second.second;

		for (auto itr = succ_begin(bb), ite = succ_end(bb); itr != ite; ++itr)
		{
			auto nextBB = *itr;
			auto bbItr = bbToNode.find(nextBB);
			if (bbItr != bbToNode.end())
				lastNode->insertEdge(bbItr->second.first);
			else
			{
				assert(nonEmptySuccMap.count(nextBB));
				auto& vec = nonEmptySuccMap[nextBB];
				for (auto succNode: vec)
					lastNode->insertEdge(succNode);
			}
		}
	}

	auto entryBlock = &cfg.getFunction()->getEntryBlock();
	if (bbToNode.count(entryBlock))
		cfg.getEntryNode()->insertEdge(bbToNode[entryBlock].first);
	else
	{
		assert(nonEmptySuccMap.count(entryBlock));
		auto& vec = nonEmptySuccMap[entryBlock];
		for (auto node: vec)
			cfg.getEntryNode()->insertEdge(node);
	}
}

void PointerProgramBuilder::constructDefUseChain(PointerCFG& cfg)
{
	for (auto useNode: cfg)
	{
		// Local helper function to draw a (potential) def-use edge from a node corresponds to defVal to useNode
		auto drawDefUseEdgeFromValue = [&cfg, useNode] (const Value* defVal)
		{
			assert(defVal != nullptr);

			// Functions are not required to be connected
			if (isa<Function>(defVal))
				return;

			if (isa<GlobalValue>(defVal) || isa<Argument>(defVal))
			{
				// Nodes that use global values are def roots
				cfg.getEntryNode()->insertDefUseEdge(useNode);
			}
			else if (auto defInst = dyn_cast<Instruction>(defVal))
			{
				// For instructions, see if we have corresponding node attached to it
				if (auto defNode = cfg.getNodeFromInstruction(defInst))
					defNode->insertDefUseEdge(useNode);
				else
				{
					errs() << "Failed to find node for instruction " << *defInst << "\n";	
				}
			}
		};

		if (isa<EntryNode>(useNode))
			continue;

		// For alloc nodes, there should not be anything up on the use-def chain. Draw an edge from starting node to it
		if (isa<AllocNode>(useNode))
		{
			cfg.getEntryNode()->insertDefUseEdge(useNode);
			continue;
		}

		auto inst = useNode->getInstruction();

		// Pointer arithmetic (inttoptr + ptrtoint) is a special case we need to handle first
		if (isa<IntToPtrInst>(inst))
		{
			auto op = inst->getOperand(0);
			Value* srcValue = nullptr;

			bool match = PatternMatch::match(op, PatternMatch::m_PtrToInt(PatternMatch::m_Value(srcValue)));
			if (!match)
				match = PatternMatch::match(op,
				PatternMatch::m_Add(
					PatternMatch::m_PtrToInt(
						PatternMatch::m_Value(srcValue)),
					PatternMatch::m_Value()));

			if (match)
			{
				drawDefUseEdgeFromValue(srcValue->stripPointerCasts());
				continue;
			}
		}

		// A use-def chain exploration is better than a def-use chain exploration because the latter may miss some edges (e.g. global vars and defs obscured by casts)
		for (auto const& u : inst->operands())
		{
			auto defVal = u.get()->stripPointerCasts();

			if (!defVal->getType()->isPointerTy())
				continue;

			drawDefUseEdgeFromValue(defVal);
		}
	}
}

PointerProgram PointerProgramBuilder::buildPointerProgram(const Module& module)
{
	auto prog = PointerProgram();

	auto dl = DataLayout(&module);
	dataLayout = &dl;

	for (auto const& f: module)
	{
		if (f.hasAddressTaken())
			prog.addAddrTakenFunction(&f);
		
		if (f.isDeclaration())
			continue;

		// Create the pointer cfg object
		auto cfg = prog.createPointerCFGForFunction(&f);
		buildPointerCFG(*cfg);
		computeNodePriority(*cfg);
		constructDefUseChain(*cfg);
	}

	dataLayout = nullptr;

	return prog;
}


}