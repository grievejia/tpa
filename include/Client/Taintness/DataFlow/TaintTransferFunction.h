#ifndef INFOFLOW_TRANSFER_FUNCTION_H
#define INFOFLOW_TRANSFER_FUNCTION_H

#include "Client/Taintness/SourceSink/Table/ExternalTaintTable.h"
#include "Client/Taintness/DataFlow/TaintStore.h"
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
	class DefUseInstruction;
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class ExternalTaintTable;
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

	tpa::EvalStatus strongUpdateStore(const tpa::MemoryLocation*, TaintLattice);
	tpa::EvalStatus weakUpdateStore(tpa::PtsSet, TaintLattice);
	std::vector<TaintLattice> collectArgumentTaintValue(llvm::ImmutableCallSite, size_t);
	bool updateParamTaintValue(const tpa::Context*, const llvm::Function*, const std::vector<TaintLattice>&);

	bool updateDirectMemoryTaint(const llvm::Value*, TaintLattice);
	bool updateReachableMemoryTaint(const llvm::Value*, TaintLattice);
	TaintLattice getTaintValueByTClass(const llvm::Value*, TClass);
	tpa::EvalStatus updateTaintValueByTClass(const llvm::Value*, TClass, TaintLattice);
	tpa::EvalStatus updateTaintCallByTPosition(const llvm::Instruction*, TPosition, TClass, TaintLattice);
	tpa::EvalStatus evalMemcpy(const llvm::Value*, const llvm::Value*);
	tpa::EvalStatus evalTaintSource(const llvm::Instruction*, const SourceTaintEntry&);
	tpa::EvalStatus evalTaintPipe(const llvm::Instruction*, const PipeTaintEntry&);
	tpa::EvalStatus evalCallBySummary(const tpa::DefUseInstruction*, const llvm::Function*, const TaintSummary&);
public:
	TaintTransferFunction(const tpa::Context*, TaintGlobalState&);
	TaintTransferFunction(const tpa::Context*, TaintStore&, TaintGlobalState&);

	tpa::EvalStatus evalAlloca(const llvm::Instruction*);
	tpa::EvalStatus evalAllOperands(const llvm::Instruction*);
	tpa::EvalStatus evalPhiNode(const llvm::Instruction*);
	tpa::EvalStatus evalStore(const llvm::Instruction*);
	tpa::EvalStatus evalLoad(const llvm::Instruction*);
	tpa::EvalStatus evalCallArguments(const llvm::Instruction*, const tpa::Context*, const llvm::Function*);

	tpa::EvalStatus evalExternalCall(const tpa::DefUseInstruction*, const llvm::Function*);
};

}	// end of taint
}	// end of client

#endif
