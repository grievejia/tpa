#include "Client/Taintness/Analysis/TaintAnalysis.h"
#include "Client/Taintness/DataFlow/TaintAnalysisEngine.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TaintAnalysis::TaintAnalysis(const tpa::PointerAnalysis& p, const ExternalPointerEffectTable& t): ptrAnalysis(p), extTable(t)
{
	sourceSinkLookupTable.readSummaryFromFile("source_sink.conf");
}

// Return true if there is a info flow violation
bool TaintAnalysis::runOnDefUseModule(const DefUseModule& duModule, bool reportError)
{
	TaintGlobalState globalState(duModule, ptrAnalysis, extTable, sourceSinkLookupTable);
	TaintAnalysisEngine engine(globalState);
	return engine.run();
}

}
}