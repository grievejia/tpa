#ifndef INFOFLOW_TRANSFER_FUNCTION_H
#define INFOFLOW_TRANSFER_FUNCTION_H

#include "Client/Taintness/SourceSink.h"
#include "Client/Taintness/TaintEnvStore.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"

#include <llvm/IR/CallSite.h>

#include <vector>

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

class TaintTransferFunction
{
private:
	SourceSinkManager ssManager;

	const tpa::PointerAnalysis& ptrAnalysis;
	tpa::ExternalPointerEffectTable extTable;

	// Return false if the check failed
	bool checkValue(const TSummary& summary, const tpa::Context*, llvm::ImmutableCallSite cs, const TaintState& state);
	bool checkValue(const TEntry& entry, tpa::ProgramLocation pLoc, const llvm::Value* val, const TaintState& state);
public:
	TaintTransferFunction(const tpa::PointerAnalysis& pa);

	std::tuple<bool, bool, bool> evalInst(const tpa::Context*, const llvm::Instruction*, TaintState&);
	std::pair<bool, bool> processLibraryCall(const tpa::Context* ctx, const llvm::Function* callee, llvm::ImmutableCallSite cs, TaintState& state);
	bool checkMemoStates(const std::unordered_map<tpa::ProgramLocation, TaintState>& memo);
};

}	// end of taint
}	// end of client

#endif
