#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefModuleAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "PointerAnalysis/DataFlow/DefUseModuleBuilder.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ExternalRefTable.h"

#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

void buildTopLevelEdges(DefUseFunction& duFunc)
{
	auto const& func = duFunc.getFunction();
	for (auto const& bb: func)
	{
		for (auto const& inst: bb)
		{
			auto duInst = duFunc.getDefUseInstruction(&inst);
			for (auto user: inst.users())
			{
				if (auto instUser = dyn_cast<Instruction>(user))
				{
					assert(inst.getParent()->getParent() == instUser->getParent()->getParent());

					auto duInstUser = duFunc.getDefUseInstruction(instUser);
					duInst->insertTopLevelEdge(duInstUser);
				}
			}
		}
	}
}

void addExternalDefs(DefUseFunction& duFunc)
{
	auto entryInst = duFunc.getEntryInst();
	for (auto const& bb: duFunc.getFunction())
	{
		for (auto const& inst: bb)
		{
			if (auto allocInst = dyn_cast<AllocaInst>(&inst))
			{
				entryInst->insertTopLevelEdge(duFunc.getDefUseInstruction(allocInst));
				continue;
			}
			if (auto retInst = dyn_cast<ReturnInst>(&inst))
			{
				auto retVal = retInst->getReturnValue();
				if (retVal == nullptr || isa<Constant>(retVal))
				{
					entryInst->insertTopLevelEdge(duFunc.getDefUseInstruction(retInst));
				}
			}
			for (auto& use: inst.operands())
			{
				auto value = use.get();
				if (isa<GlobalValue>(value) || isa<Argument>(value))
				{
					entryInst->insertTopLevelEdge(duFunc.getDefUseInstruction(&inst));
					break;
				}
			}
		}
	}	
}

void addExternalUses(DefUseFunction& duFunc, const ReachingDefMap<Instruction>& rdMap, const ModRefSummaryMap& summaryMap)
{
	auto exitInst = duFunc.getExitInst();
	auto const& summary = summaryMap.getSummary(&duFunc.getFunction());

	auto& rdStore = rdMap.getReachingDefStore(exitInst->getInstruction());

	for (auto const& mapping: rdStore)
	{
		auto loc = mapping.first;
		if (summary.isMemoryWritten(loc))
		{
			for (auto inst: mapping.second)
			{
				if (inst != nullptr)
					duFunc.getDefUseInstruction(inst)->insertMemLevelEdge(loc, exitInst);
			}
		}
	}
}

void computeNodePriority(DefUseFunction& duFunc)
{
	auto& f = duFunc.getFunction();

	size_t currPrio = 1;
	for (auto itr = po_begin(&f), ite = po_end(&f); itr != ite; ++itr)
	{
		auto bb = *itr;
		for (auto bItr = bb->rbegin(), bIte = bb->rend(); bItr != bIte; ++bItr)
		{
			auto duInst = duFunc.getDefUseInstruction(&*bItr);
			duInst->setPriority(currPrio++);
		}
	}
}

class MemReadVisitor: public llvm::InstVisitor<MemReadVisitor>
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalRefTable& extRefTable;

	using RDMapType = ReachingDefMap<Instruction>;
	const RDMapType& rdMap;

	DefUseFunction& duFunc;

	void addMemDefUseEdge(const MemoryLocation* loc, const ReachingDefStore<Instruction>& rdStore, DefUseInstruction* dstDuInst)
	{
		auto nodeSet = rdStore.getReachingDefs(loc);
		if (nodeSet == nullptr)
		{
			duFunc.getEntryInst()->insertMemLevelEdge(loc, dstDuInst);
			return;
		}
		for (auto srcInst: *nodeSet)
		{
			auto srcDuInst = srcInst == nullptr ? duFunc.getEntryInst() : duFunc.getDefUseInstruction(srcInst);
			srcDuInst->insertMemLevelEdge(loc, dstDuInst);
		}
	}

	void processMemRead(Instruction& inst, Value* ptr, bool array = false)
	{
		auto pSet = ptrAnalysis.getPtsSet(ptr);
		if (!pSet.isEmpty())
		{
			auto& rdStore = rdMap.getReachingDefStore(&inst);
			auto dstDuInst = duFunc.getDefUseInstruction(&inst);
			for (auto loc: pSet)
			{
				if (array)
				{
					for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
						addMemDefUseEdge(oLoc, rdStore, dstDuInst);
				}
				else
				{
					addMemDefUseEdge(loc, rdStore, dstDuInst);
				}
			}
		}
	}

	void processExternalCall(CallSite cs, const Function* f)
	{
		auto extType = extRefTable.lookup(f->getName());
		auto& inst = *cs.getInstruction();

		switch (extType)
		{
			case RefEffect::RefArg0:
			{
				processMemRead(inst, cs.getArgument(0));
				break;
			}
			case RefEffect::RefArg1:
			{
				processMemRead(inst, cs.getArgument(1));
				break;
			}
			case RefEffect::RefArg2:
			{
				processMemRead(inst, cs.getArgument(2));
				break;
			}
			case RefEffect::RefArg0Arg1:
			{
				processMemRead(inst, cs.getArgument(0));
				processMemRead(inst, cs.getArgument(1));
				break;
			}
			case RefEffect::RefAfterArg0:
			{
				for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
					processMemRead(inst, cs.getArgument(i));
				break;
			}
			case RefEffect::RefAfterArg1:
			{
				for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
					processMemRead(inst, cs.getArgument(i));
				break;
			}
			case RefEffect::RefAllArgs:
			{
				for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
					processMemRead(inst, cs.getArgument(i));
				break;
			}
			case RefEffect::RefArg1Array:
			{
				processMemRead(inst, cs.getArgument(1), true);
				break;
			}
			case RefEffect::UnknownEffect:
				llvm_unreachable("Unknown ref effect");
			default:
				break;
		}
	}
public:
	MemReadVisitor(const RDMapType& m, DefUseFunction& f, const PointerAnalysis& p, const ModRefSummaryMap& sm, const ExternalRefTable& e): ptrAnalysis(p), summaryMap(sm), extRefTable(e), rdMap(m), duFunc(f) {}

	// Default case
	void visitInstruction(Instruction&) {}

	void visitLoadInst(LoadInst& loadInst)
	{
		processMemRead(loadInst, loadInst.getPointerOperand());
	}

	void visitCallSite(CallSite cs)
	{
		auto inst = cs.getInstruction();
		auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), inst);
		auto& rdStore = rdMap.getReachingDefStore(inst);
		auto dstDuInst = duFunc.getDefUseInstruction(inst);
		for (auto callee: callTgts)
		{
			if (callee->isDeclaration())
			{
				processExternalCall(cs, callee);
			}
			else
			{
				auto& summary = summaryMap.getSummary(callee);
				for (auto loc: summary.mem_reads())
					addMemDefUseEdge(loc, rdStore, dstDuInst);
			}
		}
	}
};

}

void DefUseModuleBuilder::buildMemLevelEdges(DefUseFunction& duFunc)
{
	// Run the reaching def analysis
	ReachingDefModuleAnalysis rdAnalysis(ptrAnalysis, summaryMap, extModTable);
	auto rdMap = rdAnalysis.runOnFunction(duFunc.getFunction());

	MemReadVisitor memVisitor(rdMap, duFunc, ptrAnalysis, summaryMap, extRefTable);
	memVisitor.visit(const_cast<Function&>(duFunc.getFunction()));

	// Add mem-level external uses
	addExternalUses(duFunc, rdMap, summaryMap);
}

void DefUseModuleBuilder::buildDefUseFunction(DefUseFunction& duFunc)
{
	// Add top-level edges
	buildTopLevelEdges(duFunc);

	// Add top-level external defs
	addExternalDefs(duFunc);

	// Add memory-level edges
	buildMemLevelEdges(duFunc);

	// Compute node priorities;
	computeNodePriority(duFunc);
}

DefUseModule DefUseModuleBuilder::buildDefUseModule(const Module& module)
{
	DefUseModule duModule(module);

	for (auto& mapping: duModule)
		buildDefUseFunction(mapping.second);

	return duModule;
}

}