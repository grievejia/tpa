#ifndef TPA_TAINT_FUNCTION_PRECISION_LOSS_TRACKER_H
#define TPA_TAINT_FUNCTION_PRECISION_LOSS_TRACKER_H

#include "Client/Taintness/DataFlow/TaintStore.h"
#include "Client/Taintness/Precision/TrackerTypes.h"
#include "Client/Taintness/Precision/TrackingTransferFunction.h"
#include "TPA/DataFlow/TPAWorkList.h"

namespace tpa
{
	class Context;
	class DefUseInstruction;
	class DefUseFunction;
}

namespace client
{
namespace taint
{

class TaintGlobalState;
class PrecisionLossGlobalState;

class FunctionPrecisionLossTracker
{
private:
	const tpa::Context* ctx;
	const tpa::DefUseFunction* duFunc;

	const TaintGlobalState& globalState;
	PrecisionLossGlobalState& precGlobalState;

	GlobalWorkList& globalWorkList;
	LocalWorkList& localWorkList;

	TrackingTransferFunction transferFunction;

	void evalInst(const tpa::DefUseInstruction*, const TaintStore&);
	void evalEntryInst(const tpa::DefUseInstruction*);
	void evalCallInst(const tpa::DefUseInstruction*);

	void evalExternalCallInst(const tpa::DefUseInstruction*, const llvm::Function*);
public:
	FunctionPrecisionLossTracker(const tpa::Context*, const tpa::DefUseFunction*, const TaintGlobalState&, PrecisionLossGlobalState&, GlobalWorkList&);

	void track();
};

}
}

#endif
