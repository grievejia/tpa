#ifndef INFOFLOW_TRANSFER_FUNCTION_H
#define INFOFLOW_TRANSFER_FUNCTION_H

#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "Utils/Hashing.h"

#include <llvm/IR/CallSite.h>

#include <vector>
#include <unordered_set>

namespace llvm
{
	class Instruction;
}

namespace tpa
{
	class Context;
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class SourceSinkLookupTable;
class TaintGlobalState;

class TaintTransferFunction
{
private:
	const tpa::PointerAnalysis& ptrAnalysis;
	const tpa::ExternalPointerEffectTable& extTable;
	const SourceSinkLookupTable& sourceSinkLookupTable;

	// Stash uloc and nloc here to grab them quickly during analysis
	const tpa::MemoryLocation *uLoc, *nLoc;
public:
	TaintTransferFunction(TaintGlobalState& g);

	std::tuple<bool, bool, bool> evalInst(const tpa::Context*, const llvm::Instruction*, TaintEnv&, TaintStore&);
	std::tuple<bool, bool, bool> processLibraryCall(const tpa::Context* ctx, const llvm::Function* callee, llvm::ImmutableCallSite cs, TaintEnv&, TaintStore&);
};

}	// end of taint
}	// end of client

#endif
