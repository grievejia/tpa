#ifndef TPA_TAINT_TRACKING_TRANSFER_FUNCTION_H
#define TPA_TAINT_TRACKING_TRANSFER_FUNCTION_H

#include "Client/Lattice/TaintLattice.h"
#include "Client/Taintness/Precision/TrackerTypes.h"

namespace llvm
{
	class Function;
	class Instruction;
}

namespace tpa
{
	class Context;
	class DefUseInstruction;
	class PtsSet;
}

namespace client
{
namespace taint
{

class TaintGlobalState;

class TrackingTransferFunction
{
private:
	const TaintGlobalState& globalState;
	const tpa::Context* ctx;

	TaintLattice getTaintForValue(const llvm::Value*);

	MemorySet evalPtsSet(const tpa::DefUseInstruction*, const tpa::PtsSet&);
public:
	TrackingTransferFunction(const TaintGlobalState&, const tpa::Context*);

	ValueSet evalAllOperands(const tpa::DefUseInstruction*);
	ValueSet evalStore(const tpa::DefUseInstruction*);
	MemorySet evalLoad(const tpa::DefUseInstruction*);
};

}
}

#endif
