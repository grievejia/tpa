#ifndef TPA_TAINT_PRECISION_LOSS_TRACKER_H
#define TPA_TAINT_PRECISION_LOSS_TRACKER_H

#include "Client/Taintness/SourceSink/SinkViolationClassifier.h"
#include "Client/Taintness/Precision/TrackerWorkList.h"

#include <llvm/ADT/SmallPtrSet.h>

namespace client
{
namespace taint
{

class ContextDefUseFunction;
class TaintGlobalState;

class PrecisionLossTracker
{
private:
	using CallSiteSet = std::unordered_set<tpa::ProgramLocation>;

	const TaintGlobalState& globalState;
	const tpa::Context* ctx;
	const tpa::DefUseFunction* duFunc;

	using ValueSet = llvm::SmallPtrSet<const llvm::Value*, 8>;
	using MemorySet = llvm::SmallPtrSet<const tpa::MemoryLocation*, 8>;
	using ArgumentSet = llvm::SmallPtrSet<const llvm::Argument*, 8>;
	using DefUseInstructionSet = llvm::SmallPtrSet<const tpa::DefUseInstruction*, 64>;

	ArgumentSet trackedArguments;
	MemorySet trackedExternalLocations;
	DefUseInstructionSet visitedDuInstructions;

	TrackerWorkList workList;

	void processSinkViolationRecords(const tpa::DefUseInstruction*, const SinkViolationRecords&);
	void trackSource(const tpa::DefUseInstruction*);
	void trackValues(const tpa::DefUseInstruction*, const ValueSet&);
	void trackMemory(const tpa::DefUseInstruction*, const MemorySet&);

	CallSiteSet trackCallSite();
	void trackCallSiteByValue(const CallSiteSet&, CallSiteSet&);
public:
	PrecisionLossTracker(const ContextDefUseFunction&, const FunctionSinkViolationRecords&, const TaintGlobalState&);

	CallSiteSet trackPrecisionLossSource();
};

}
}

#endif
