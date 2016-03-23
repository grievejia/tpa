#include "Annotation/ModRef/ExternalModRefTable.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "TaintAnalysis/FrontEnd/ModRefModuleSummary.h"
#include "TaintAnalysis/FrontEnd/ReachingDefModuleAnalysis.h"
#include "Util/DataStructure/FIFOWorkList.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;
using namespace tpa;

namespace taint
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

class EvalVisitor: public InstVisitor<EvalVisitor>
{
private:
	const SemiSparsePointerAnalysis& ptrAnalysis;
	const ModRefModuleSummary& summaryMap;
	const ExternalModRefTable& modRefTable;

	using StoreType = ReachingDefStore<Instruction>;
	StoreType store;

	void evalStore(Instruction& inst, Value* ptr)
	{
		auto pSet = ptrAnalysis.getPtsSet(ptr);
		bool needWeekUpdate = true;
		if (pSet.size() == 1)
		{
			auto obj = *pSet.begin();
			if (!obj->isSummaryObject())
			{
				store.updateBinding(obj, &inst);
				needWeekUpdate = false;
			}
		}
		if (needWeekUpdate)
		{
			for (auto obj: pSet)
				store.insertBinding(obj, &inst);
		}
	}

	void modValue(const Value* v, const Instruction* inst, bool isReachableMemory)
	{
		auto pSet = ptrAnalysis.getPtsSet(v);
		for (auto loc: pSet)
		{
			if (isReachableMemory)
			{
				for (auto oLoc: ptrAnalysis.getMemoryManager().getReachableMemoryObjects(loc))
					store.insertBinding(oLoc, inst);
			}
			else
			{
				store.insertBinding(loc, inst);
			}
			
		}
	}

	void evalExternalCall(CallSite cs, const Function* f)
	{
		auto summary = modRefTable.lookup(f->getName());
		if (summary == nullptr)
		{
			errs() << "Missing entry in ModRefTable: " << f->getName() << "\n";
			llvm_unreachable("Consider adding the function to modref annotations");
		}

		for (auto const& effect: *summary)
		{
			if (effect.isModEffect())
			{
				auto const& pos = effect.getPosition();
				if (pos.isReturnPosition())
				{
					modValue(cs.getInstruction()->stripPointerCasts(), cs.getInstruction(), effect.onReachableMemory());
				}
				else
				{
					auto const& argPos = pos.getAsArgPosition();
					unsigned idx = argPos.getArgIndex();

					if (!argPos.isAfterArgPosition())
						modValue(cs.getArgument(idx)->stripPointerCasts(), cs.getInstruction(), effect.onReachableMemory());
					else
					{
						for (auto i = idx, e = cs.arg_size(); i < e; ++i)
							modValue(cs.getArgument(i)->stripPointerCasts(), cs.getInstruction(), effect.onReachableMemory());
					}
				}
			}
		}
	}
public:
	EvalVisitor(const SemiSparsePointerAnalysis& p, const ModRefModuleSummary& sm, const ExternalModRefTable& e, const StoreType& s): ptrAnalysis(p), summaryMap(sm), modRefTable(e), store(s) {}
	const StoreType& getStore() const { return store; }

	// Default case
	void visitInstruction(Instruction&) {}

	void visitAllocaInst(AllocaInst& allocInst)
	{
		auto pSet = ptrAnalysis.getPtsSet(&allocInst);
		for (auto obj: pSet)
			store.insertBinding(obj, &allocInst);
	}

	void visitStoreInst(StoreInst& storeInst)
	{
		auto ptr = storeInst.getPointerOperand();
		evalStore(storeInst, ptr);
	}

	void visitCallSite(CallSite cs)
	{
		auto callTgts = ptrAnalysis.getCallees(cs);
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

	util::FIFOWorkList<const Instruction*> workList;
	workList.enqueue(entryInst);

	while (!workList.empty())
	{
		auto inst = workList.dequeue();
		
		EvalVisitor evalVisitor(ptrAnalysis, summaryMap, modRefTable, rdMap.getReachingDefStore(inst));
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