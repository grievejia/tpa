#include "Client/Taintness/Analysis/TaintAnalysis.h"
#include "Client/Taintness/DataFlow/TaintAnalysisEngine.h"
#include "Client/Taintness/DataFlow/TaintGlobalState.h"

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

// Return true if there is a info flow violation
bool TaintAnalysis::runOnDefUseModule(const DefUseModule& duModule, bool reportError)
{
	TaintGlobalState globalState(duModule, ptrAnalysis);
	TaintAnalysisEngine engine(globalState, extTable);
	return engine.run();
}

}
}