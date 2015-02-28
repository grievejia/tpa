#ifndef TPA_TAINT_ANALYSIS_H
#define TPA_TAINT_ANALYSIS_H

#include "Client/Taintness/TaintEnvStore.h"
#include "Client/Taintness/TaintTransferFunction.h"
#include "Client/Util/ClientWorkList.h"
#include "MemoryModel/Precision/ProgramLocation.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/CallSite.h>

#include <unordered_map>

namespace client
{
namespace taint
{

class TaintAnalysis
{
private:
	std::unordered_map<tpa::ProgramLocation, TaintState> memo;

	const tpa::PointerAnalysis& ptrAnalysis;
	TaintTransferFunction transferFunction;

	// Maps from callee to its callsites
	std::unordered_multimap<const llvm::Function*, tpa::ProgramLocation> returnMap;

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

	ClientWorkList initializeWorkList(const llvm::Module& module);
	void evalFunction(const tpa::Context* ctx, const llvm::Function* f, ClientWorkList& workList, const llvm::Module& module);
	void applyFunction(const tpa::Context* ctx, llvm::ImmutableCallSite, const llvm::Function* f, const TaintState&, ClientWorkList::LocalWorkList&, ClientWorkList&);
	void evalReturn(const tpa::Context* ctx, const llvm::Instruction* inst, const TaintState& state, ClientWorkList& workList);
	void propagateStateChange(const tpa::Context* ctx, const llvm::Instruction* inst, const TaintState& state, ClientWorkList::LocalWorkList& workList);
public:
	TaintAnalysis(const tpa::PointerAnalysis& p): ptrAnalysis(p), transferFunction(ptrAnalysis) {}

	// Return true if there is a info flow violation
	bool runOnModule(const llvm::Module& M);
};

}
}

#endif
