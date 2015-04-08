#ifndef INFOFLOW_TRANSFER_FUNCTION_H
#define INFOFLOW_TRANSFER_FUNCTION_H

#include "Client/Taintness/DataFlow/SourceSink.h"
#include "Client/Taintness/DataFlow/TaintEnvStore.h"
#include "MemoryModel/PtsSet/PtsSet.h"
#include "TPA/DataFlow/EvalStatus.h"

#include <llvm/IR/CallSite.h>

#include <vector>
#include <unordered_set>
#include <MemoryModel/Memory/Memory.h>

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
	const tpa::Context* ctx;
	TaintStore* store;
	TaintGlobalState& globalState;

	TaintLattice getTaintForValue(const llvm::Value*);
	TaintLattice getTaintForOperands(const llvm::Instruction*);
	TaintLattice loadTaintFromPtsSet(tpa::PtsSet);

	tpa::EvalStatus evalMemcpy(const llvm::Instruction*);
	tpa::EvalStatus evalMalloc(const llvm::Instruction*);
	tpa::EvalStatus evalCallBySummary(const llvm::Instruction*, const TSummary&);

	tpa::EvalStatus strongUpdateStore(const tpa::MemoryLocation*, TaintLattice);
	tpa::EvalStatus weakUpdateStore(tpa::PtsSet, TaintLattice);
	std::vector<TaintLattice> collectArgumentTaintValue(llvm::ImmutableCallSite, size_t);
	bool updateParamTaintValue(const tpa::Context*, const llvm::Function*, const std::vector<TaintLattice>&);

	bool memcpyTaint(tpa::PtsSet, tpa::PtsSet);
	bool copyTaint(const llvm::Value*, const llvm::Value*);
	tpa::EvalStatus taintValueByTClass(const llvm::Value*, TClass, TaintLattice);
	tpa::EvalStatus taintCallByEntry(const llvm::Instruction*, const TEntry&);
public:
	TaintTransferFunction(const tpa::Context*, TaintGlobalState&);
	TaintTransferFunction(const tpa::Context*, TaintStore&, TaintGlobalState&);

	tpa::EvalStatus evalAlloca(const llvm::Instruction*);
	tpa::EvalStatus evalAllOperands(const llvm::Instruction*);
	tpa::EvalStatus evalPhiNode(const llvm::Instruction*);
	tpa::EvalStatus evalStore(const llvm::Instruction*);
	tpa::EvalStatus evalLoad(const llvm::Instruction*);
	tpa::EvalStatus evalCallArguments(const llvm::Instruction*, const tpa::Context*, const llvm::Function*);

	tpa::EvalStatus evalExternalCall(const llvm::Instruction*, const llvm::Function*);
};

}	// end of taint
}	// end of client

#endif
