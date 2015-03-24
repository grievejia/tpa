#ifndef TPA_TAINT_ANALYSIS_H
#define TPA_TAINT_ANALYSIS_H

#include "Client/Taintness/TaintEnvStore.h"
#include "Client/Taintness/TaintTransferFunction.h"
#include "MemoryModel/Precision/ProgramLocation.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"
#include "TPA/DataFlow/TPAWorkList.h"

#include <llvm/IR/CallSite.h>

#include <unordered_set>
#include <unordered_map>

namespace tpa
{
	class ModRefSummaryMap;
}

namespace client
{
namespace taint
{

class TaintAnalysis
{
private:
	using ClientWorkList = tpa::TPAWorkList<tpa::DefUseFunction>;

	std::unordered_map<tpa::ProgramLocation, TaintStore> memo;

	const tpa::PointerAnalysis& ptrAnalysis;
	const tpa::ModRefSummaryMap& summaryMap;

	TaintEnv env;
	TaintTransferFunction transferFunction;

	std::unordered_set<tpa::ProgramLocation> visitedFuncs;

	// Helper function to insert memo
	template <typename StateType>
	bool insertMemoState(const tpa::ProgramLocation& pLoc, StateType&& s)
	{
		auto itr = memo.find(pLoc);
		if (itr == memo.end())
		{
			memo.insert(std::make_pair(pLoc, std::forward<StateType>(s)));
			return true;
		}
		else
		{
			return itr->second.mergeWith(s);
		}
	}
	bool insertMemoState(const tpa::ProgramLocation& pLoc, const tpa::MemoryLocation* loc, TaintLattice tVal)
	{
		bool changed = false;
		auto itr = memo.find(pLoc);
		if (itr == memo.end())
		{
			itr = memo.insert(std::make_pair(pLoc, TaintStore())).first;
			changed = true;
		}
		
		return itr->second.weakUpdate(loc, tVal);
	}

	ClientWorkList initializeWorkList(const tpa::DefUseModule& module);
	void evalFunction(const tpa::Context* ctx, const tpa::DefUseFunction* f, ClientWorkList& workList, const tpa::DefUseModule& module);
	void applyFunction(const tpa::Context* oldCtx, const tpa::Context* newCtx, llvm::ImmutableCallSite, const tpa::DefUseFunction* f, const TaintStore&, ClientWorkList&);
	void evalReturn(const tpa::Context* ctx, const llvm::Instruction* inst, TaintEnv& env, const TaintStore& store, const tpa::DefUseModule& duModule, ClientWorkList& workList);
	void propagateStateChange(const tpa::Context* ctx, const tpa::DefUseInstruction* inst, bool envChanged, bool storeChanged, const TaintStore& store, ClientWorkList::LocalWorkList& workList);
public:
	TaintAnalysis(const tpa::PointerAnalysis& p, const tpa::ModRefSummaryMap& s, const tpa::ExternalPointerEffectTable& t): ptrAnalysis(p), summaryMap(s), transferFunction(ptrAnalysis, t) {}

	// Return true if there is a info flow violation
	bool runOnDefUseModule(const tpa::DefUseModule& m);
};

}
}

#endif
