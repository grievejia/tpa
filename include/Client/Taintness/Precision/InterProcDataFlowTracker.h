#ifndef TPA_TAINT_INTER_PROC_DATAFLOW_TRACKER_H
#define TPA_TAINT_INTER_PROC_DATAFLOW_TRACKER_H

#include "Client/Lattice/TaintLattice.h"
#include "Client/Taintness/Precision/TrackerTypes.h"

namespace tpa
{
	class DefUseProgramLocation;
}

namespace client
{
namespace taint
{

class TaintGlobalState;
class PrecisionLossGlobalState;

class InterProcDataFlowTracker
{
private:
	const TaintGlobalState& globalState;
	PrecisionLossGlobalState& precGlobalState;
	GlobalWorkList& workList;
protected:
	using TaintVector = std::vector<TaintLattice>;
	using IndexVector = std::vector<size_t>;

	InterProcDataFlowTracker(const TaintGlobalState&, PrecisionLossGlobalState&, GlobalWorkList&);

	TaintLattice getTaintValue(const tpa::Context*, const llvm::Value*);
	TaintLattice getTaintValue(const tpa::DefUseProgramLocation&, const tpa::MemoryLocation*);

	IndexVector getDemandingIndices(const TaintVector&);
	IndexVector getImpreciseIndices(const TaintVector&);

	template <typename InputVector, typename UnaryOperator, typename OutputVector = std::vector<std::remove_reference_t<decltype(std::declval<UnaryOperator>()(std::declval<typename InputVector::value_type>()))>>>
	OutputVector vectorTransform(const InputVector& input, UnaryOperator&& op)
	{
		OutputVector output;
		output.reserve(input.size());

		std::transform(input.begin(), input.end(), std::back_inserter(output), std::forward<UnaryOperator>(op));

		return output;
	}

	const tpa::DefUseFunction& getDefUseFunction(const llvm::Function*);
	const tpa::DefUseFunction& getDefUseFunction(const llvm::Instruction*);
	void enqueueWorkList(const tpa::Context*, const tpa::DefUseFunction*, const tpa::DefUseInstruction*);
	void addImprecisionSource(const tpa::DefUseProgramLocation&);
};

class ReturnDataFlowTracker: public InterProcDataFlowTracker
{
private:
	using CallTargetVector = std::vector<std::pair<const tpa::Context*, const llvm::Function*>>;
	using ReturnVector = std::vector<tpa::DefUseProgramLocation>;

	ReturnVector getReturnInsts(const CallTargetVector&);
	
	TaintVector getReturnTaintValues(const ReturnVector&);
	TaintVector getReturnTaintValues(const tpa::MemoryLocation*, const ReturnVector&);

	bool isCallSiteNeedMorePrecision(const TaintVector&);
	void propagateReturns(const ReturnVector&, const TaintVector&);
	void processReturns(const tpa::DefUseProgramLocation&, const ReturnVector&, const TaintVector&);

	void trackValue(const tpa::DefUseProgramLocation&, const ReturnVector&);
	void trackMemory(const tpa::DefUseProgramLocation&, const tpa::MemoryLocation*, const ReturnVector&);
	void trackMemories(const tpa::DefUseProgramLocation&, const ReturnVector&);
public:
	ReturnDataFlowTracker(const TaintGlobalState&, PrecisionLossGlobalState&, GlobalWorkList&);

	void trackReturn(const tpa::DefUseProgramLocation&, const CallTargetVector&);
};

class CallDataFlowTracker: public InterProcDataFlowTracker
{
private:
	using CallerVector = std::vector<std::pair<const tpa::Context*, const llvm::Instruction*>>;
	using DefUseCallerVector = std::vector<tpa::DefUseProgramLocation>;

	DefUseCallerVector getDefUseCallerVector(const CallerVector&);

	TaintVector getArgTaintValues(const DefUseCallerVector&, size_t);
	TaintVector getMemoryTaintValues(const DefUseCallerVector&, const tpa::MemoryLocation*);
	void processCaller(const DefUseCallerVector&, const TaintVector&);

	void trackValue(const tpa::DefUseFunction*, const DefUseCallerVector&);
	void trackMemory(const tpa::MemoryLocation*, const DefUseCallerVector&);
	void trackMemories(const tpa::DefUseProgramLocation&, const DefUseCallerVector&);
public:
	CallDataFlowTracker(const TaintGlobalState&, PrecisionLossGlobalState&, GlobalWorkList&);

	void trackCall(const tpa::DefUseProgramLocation&, const tpa::DefUseFunction*, const CallerVector&);
};

}
}

#endif
