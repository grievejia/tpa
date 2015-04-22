#include "Client/Taintness/Analysis/PrecisionLossTrackingAnalysis.h"
#include "Client/Taintness/DataFlow/TaintAnalysisEngine.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/PrecisionLossTracker.h"
#include "Client/Taintness/SourceSink/Checker/SinkViolationChecker.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

PrecisionLossTrackingAnalysis::PrecisionLossTrackingAnalysis(const tpa::PointerAnalysis& p): ptrAnalysis(p)
{
}

void PrecisionLossTrackingAnalysis::checkSinkViolation(const TaintGlobalState& globalState)
{
	PrecisionLossTracker tracker(globalState);
	for (auto const& sinkSignature: globalState.getSinkSignatures())
	{
		auto optStore = globalState.getMemo().lookup(sinkSignature.getCallSite());
		auto const& store = (optStore == nullptr) ? TaintStore() : *optStore;

		auto checkResult = SinkViolationChecker(globalState.getEnv(), store, sourceSinkLookupTable, ptrAnalysis).checkSinkViolation(sinkSignature);

		if (!checkResult.empty())
		{
			tracker.addSinkViolation(sinkSignature.getCallSite(), checkResult);
		}
	}

	auto impSources = tracker.trackImprecisionSource();
	for (auto const& pLoc: impSources)
	{
		errs() << *pLoc.getContext() << "::" << *pLoc.getDefUseInstruction()->getInstruction() << "\n";
	}
}

void PrecisionLossTrackingAnalysis::runOnDefUseModule(const DefUseModule& duModule)
{
	TaintGlobalState globalState(duModule, ptrAnalysis);
	TaintAnalysisEngine engine(globalState);
	engine.run();

	checkSinkViolation(globalState);
}

}
}