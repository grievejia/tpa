#ifndef TPA_TAINT_TRACKING_TRANSFER_FUNCTION_H
#define TPA_TAINT_TRACKING_TRANSFER_FUNCTION_H

#include <llvm/ADT/SmallPtrSet.h>

namespace tpa
{
	class DefUseInstruction;
}

namespace client
{
namespace taint
{

class TaintGlobalState;

class TrackingTransferFunction
{
private:
	using ValueSet = llvm::SmallPtrSet<const llvm::Value*, 8>;
	using MemorySet = llvm::SmallPtrSet<const tpa::MemoryLocation*, 16>;

	const TaintGlobalState& globalState;
	ValueSet& valueSet;
	MemorySet& memSet;
public:
	TrackingTransferFunction(const TaintGlobalState&, ValueSet&, MemorySet&);

	void eval(const tpa::DefUseInstruction*);
};

}
}

#endif
