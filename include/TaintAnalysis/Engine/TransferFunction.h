#pragma once

#include "Annotation/Taint/TaintDescriptor.h"
#include "PointerAnalysis/Support/PtsSet.h"
#include "TaintAnalysis/Engine/EvalResult.h"

namespace annotation
{
	class PipeTaintEntry;
	class SourceTaintEntry;
	class TaintSummary;
}

namespace llvm
{
	class Function;
	class ImmutableCallSite;
	class Instruction;
}

namespace tpa
{
	class FunctionContext;
}

namespace taint
{

class ProgramPoint;
class TaintGlobalState;
class TaintLocalState;
class TaintValue;

class TransferFunction
{
private:
	// The global state
	TaintGlobalState& globalState;

	// The local state
	const TaintStore* localState;

	void addTopLevelSuccessors(const ProgramPoint&, EvalResult&);
	void addMemLevelSuccessors(const ProgramPoint&, EvalResult&);
	void addMemLevelSuccessors(const ProgramPoint&, const tpa::MemoryObject*, EvalResult&);
	TaintLattice getTaintForOperands(const context::Context*, const llvm::Instruction*);
	TaintLattice loadTaintFromPtsSet(tpa::PtsSet, const TaintStore&);
	void strongUpdateStore(const tpa::MemoryObject*, TaintLattice, TaintStore&);
	void weakUpdateStore(tpa::PtsSet, TaintLattice, TaintStore&);

	std::vector<TaintLattice> collectArgumentTaintValue(const context::Context*, const llvm::ImmutableCallSite&, size_t);
	bool updateParamTaintValue(const context::Context*, const llvm::Function*, const std::vector<TaintLattice>&);
	void evalInternalCall(const ProgramPoint&, const tpa::FunctionContext&, EvalResult&, bool);
	void applyReturn(const ProgramPoint&, TaintLattice, EvalResult&);

	void updateDirectMemoryTaint(const TaintValue&, TaintLattice, const ProgramPoint&, EvalResult&);
	void updateReachableMemoryTaint(const TaintValue&, TaintLattice, const ProgramPoint&, EvalResult&);
	void updateTaintValueByTClass(const TaintValue&, annotation::TClass, TaintLattice, const ProgramPoint&, EvalResult&);
	void updateTaintCallByTPosition(const ProgramPoint&, annotation::TPosition, annotation::TClass, TaintLattice, EvalResult&);
	void evalTaintSource(const ProgramPoint&, const annotation::SourceTaintEntry&, EvalResult&);
	TaintLattice getTaintValueByTClass(const TaintValue&, annotation::TClass);
	void evalMemcpy(const TaintValue&, const TaintValue&, const ProgramPoint&, EvalResult&);
	void evalTaintPipe(const ProgramPoint&, const annotation::PipeTaintEntry&, EvalResult&);
	void evalCallBySummary(const ProgramPoint&, const llvm::Function*, const annotation::TaintSummary&, EvalResult&);
	void evalExternalCall(const ProgramPoint&, const llvm::Function*, EvalResult&);

	void evalEntry(const ProgramPoint&, EvalResult&, bool);
	void evalAlloca(const ProgramPoint&, EvalResult&);
	void evalAllOperands(const ProgramPoint&, EvalResult&);
	void evalLoad(const ProgramPoint&, EvalResult&);
	void evalStore(const ProgramPoint&, EvalResult&);
	void evalCall(const ProgramPoint&, EvalResult&);
	void evalReturn(const ProgramPoint&, EvalResult&);
public:
	TransferFunction(TaintGlobalState& g, const TaintStore* l): globalState(g), localState(l) {}

	EvalResult eval(const ProgramPoint&);
};

}