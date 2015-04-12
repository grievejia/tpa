#include "Client/Taintness/Analysis/PrecisionLossTrackingAnalysis.h"
#include "Client/Taintness/DataFlow/TaintAnalysisEngine.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"
#include "Client/Taintness/Precision/PrecisionLossTracker.h"
#include "Client/Taintness/SourceSink/SinkViolationChecker.h"
#include "Client/Taintness/SourceSink/SinkViolationClassifier.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

PrecisionLossTrackingAnalysis::PrecisionLossTrackingAnalysis(const tpa::PointerAnalysis& p, const ExternalPointerEffectTable& t): ptrAnalysis(p), extTable(t)
{
	sourceSinkLookupTable.readSummaryFromFile("source_sink.conf");
}

void PrecisionLossTrackingAnalysis::checkSinkViolation(const TaintGlobalState& globalState)
{
	SinkViolationClassifier classifier(globalState.getProgram());
	for (auto const& sinkSignature: globalState.getSinkSignatures())
	{
		auto optStore = globalState.getMemo().lookup(sinkSignature.getCallSite());
		auto const& store = (optStore == nullptr) ? TaintStore() : *optStore;

		auto checkResult = SinkViolationChecker(globalState.getEnv(), store, sourceSinkLookupTable, ptrAnalysis).checkSinkViolation(sinkSignature);

		if (!checkResult.empty())
		{
			classifier.addSinkSignature(sinkSignature.getCallSite(), checkResult);
		}
	}

	for (auto const& mapping: classifier)
	{
		auto const& ctxDuFunc = mapping.first;
		auto const& funRecords = mapping.second;
		errs() << "Tracking " << *ctxDuFunc.getContext() << "::" << ctxDuFunc.getDefUseFunction()->getFunction().getName() << "...\n";

		PrecisionLossTracker tracker(ctxDuFunc, funRecords, globalState);
		tracker.trackPrecisionLossSource();
	}
}

void PrecisionLossTrackingAnalysis::runOnDefUseModule(const DefUseModule& duModule)
{
	TaintGlobalState globalState(duModule, ptrAnalysis, extTable, sourceSinkLookupTable);
	TaintAnalysisEngine engine(globalState);
	engine.run();

	checkSinkViolation(globalState);
}

}
}