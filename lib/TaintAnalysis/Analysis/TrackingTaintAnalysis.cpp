#include "TaintAnalysis/Analysis/TrackingTaintAnalysis.h"
#include "TaintAnalysis/Engine/Initializer.h"
#include "TaintAnalysis/Engine/SinkViolationChecker.h"
#include "TaintAnalysis/Engine/TaintPropagator.h"
#include "TaintAnalysis/Engine/TaintGlobalState.h"
#include "TaintAnalysis/Engine/TransferFunction.h"
#include "TaintAnalysis/Precision/PrecisionLossTracker.h"
#include "Util/AnalysisEngine/DataFlowAnalysis.h"

namespace taint
{

std::pair<bool, ProgramPointSet> TrackingTaintAnalysis::runOnDefUseModule(const DefUseModule& duModule)
{
	auto globalState = TaintGlobalState(duModule, ptrAnalysis, extTable, env, memo);
	auto dfa = util::DataFlowAnalysis<TaintGlobalState, TaintMemo, TransferFunction, TaintPropagator>(globalState, memo);
	dfa.runOnInitialState<Initializer>(TaintStore());

	auto violationRecord = SinkViolationChecker(env, memo, extTable, ptrAnalysis).checkSinkViolation(globalState.getSinks());

	PrecisionLossTracker tracker(globalState);
	return std::make_pair(violationRecord.empty(), tracker.trackImprecision(violationRecord));
}

}