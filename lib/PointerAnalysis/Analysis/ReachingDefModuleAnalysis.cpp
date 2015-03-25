#include "MemoryModel/Memory/MemoryManager.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/Analysis/ReachingDefModuleAnalysis.h"
#include "PointerAnalysis/DataFlow/ModRefSummary.h"
#include "PointerAnalysis/External/ExternalModTable.h"
#include "Utils/WorkList.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

const Instruction* getNextInstruction(const Instruction* inst)
{
	BasicBlock::const_iterator itr = inst;
	++itr;
	assert(itr != inst->getParent()->end());
	return itr;
}

class EvalVisitor: public llvm::InstVisitor<EvalVisitor>
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModTable& extModTable;

	using StoreType = ReachingDefStore<Instruction>;
	StoreType store;

	void evalStore(Instruction& inst, Value* ptr)
	{
		if (auto pSet = ptrAnalysis.getPtsSet(ptr))
		{
			bool needWeekUpdate = true;
			if (pSet->getSize() == 1)
			{
				auto loc = *pSet->begin();
				if (!loc->isSummaryLocation())
				{
					store.updateBinding(*pSet->begin(), &inst);
					needWeekUpdate = false;
				}
			}
			if (needWeekUpdate)
			{
				for (auto loc: *pSet)
					store.insertBinding(loc, &inst);
			}
		}
	}

	void evalExternalCall(CallSite cs, const Function* f)
	{
		auto extType = extModTable.lookup(f->getName());
		auto modArg = [this, cs] (const Value* v, bool array = false)
		{
			if (auto pSet = ptrAnalysis.getPtsSet(v))
			{
				for (auto loc: *pSet)
				{
					if (array)
					{
						for (auto oLoc: ptrAnalysis.getMemoryManager().getAllOffsetLocations(loc))
							store.insertBinding(oLoc, cs.getInstruction());
					}
					else
					{
						store.insertBinding(loc, cs.getInstruction());
					}
					
				}
			}
		};
		switch (extType)
		{
			case ModEffect::ModArg0:
			{
				modArg(cs.getArgument(0));
				break;
			}
			case ModEffect::ModArg1:
			{
				modArg(cs.getArgument(1));
				break;
			}
			case ModEffect::ModAfterArg0:
			{
				for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
					modArg(cs.getArgument(i));
				break;
			}
			case ModEffect::ModAfterArg1:
			{
				for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
					modArg(cs.getArgument(i));
				break;
			}
			case ModEffect::ModArg0Array:
			{
				modArg(cs.getArgument(0), true);
				break;
			}
			case ModEffect::UnknownEffect:
				llvm_unreachable("Unknown mod effect");
			default:
				break;
		}
	}
public:
	EvalVisitor(const PointerAnalysis& p, const ModRefSummaryMap& sm, const ExternalModTable& e, const StoreType& s): ptrAnalysis(p), summaryMap(sm), extModTable(e), store(s) {}
	const StoreType& getStore() const { return store; }

	// Default case
	void visitInstruction(Instruction&) {}

	void visitAllocaInst(AllocaInst& allocInst)
	{
		auto pSet = ptrAnalysis.getPtsSet(&allocInst);
		assert(pSet != nullptr && pSet->getSize() == 1);
		store.insertBinding(*pSet->begin(), &allocInst);
	}

	void visitStoreInst(StoreInst& storeInst)
	{
		auto ptr = storeInst.getPointerOperand();
		evalStore(storeInst, ptr);
	}

	void visitCallSite(CallSite cs)
	{
		auto callTgts = ptrAnalysis.getCallTargets(Context::getGlobalContext(), cs.getInstruction());
		for (auto callee: callTgts)
		{
			if (callee->isDeclaration())
			{
				evalExternalCall(cs, callee);
			}
			else
			{
				auto& summary = summaryMap.getSummary(callee);
				for (auto loc: summary.mem_writes())
					store.insertBinding(loc, cs.getInstruction());
			}
		}
	}
};

}	// end of anonymous namespace

ReachingDefMap<Instruction> ReachingDefModuleAnalysis::runOnFunction(const Function& func)
{
	ReachingDefMap<Instruction> rdMap;

	auto entryInst = func.getEntryBlock().begin();
	auto& summary = summaryMap.getSummary(&func);
	auto& initStore = rdMap.getReachingDefStore(entryInst);
	for (auto loc: summary.mem_reads())
		initStore.insertBinding(loc, nullptr);

	WorkList<const Instruction*> workList;
	workList.enqueue(entryInst);

	while (!workList.isEmpty())
	{
		auto inst = workList.dequeue();
		
		EvalVisitor evalVisitor(ptrAnalysis, summaryMap, extModTable, rdMap.getReachingDefStore(inst));
		evalVisitor.visit(const_cast<Instruction&>(*inst));

		if (auto termInst = dyn_cast<TerminatorInst>(inst))
		{
			for (auto i = 0u, e = termInst->getNumSuccessors(); i != e; ++i)
			{
				auto succInst = termInst->getSuccessor(i)->begin();
				if (rdMap.update(succInst, evalVisitor.getStore()))
					workList.enqueue(succInst);
			}
		}
		else
		{
			auto nextInst = getNextInstruction(inst);
			if (rdMap.update(nextInst, evalVisitor.getStore()))
				workList.enqueue(nextInst);
		}
	}

	return rdMap;
}

}
