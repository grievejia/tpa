#ifndef TPA_TAINT_TRACKING_TRANSFER_FUNCTION_H
#define TPA_TAINT_TRACKING_TRANSFER_FUNCTION_H

#include "Client/Lattice/TaintLattice.h"

#include <llvm/ADT/SmallPtrSet.h>

namespace llvm
{
	class Function;
	class Instruction;
	class Value;
}

namespace tpa
{
	class Context;
	class DefUseInstruction;
	class MemoryLocation;
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
	using ValueSet = llvm::SmallPtrSet<const llvm::Value*, 8>;
	using MemorySet = llvm::SmallPtrSet<const tpa::MemoryLocation*, 8>;

	const TaintGlobalState& globalState;
	const tpa::Context* ctx;
	ValueSet& valueSet;
	MemorySet& memSet;

	TaintLattice getTaintForValue(const llvm::Value*);

	void evalAllOperands(const llvm::Instruction*);
	void evalStore(const llvm::Instruction*);
	void evalLoad(const llvm::Instruction*);

	template <typename SetType>
	void evalPtsSet(const llvm::Instruction*, const SetType&);

	void evalCall(const tpa::DefUseInstruction*);
	void evalExternalCall(const tpa::DefUseInstruction*, const llvm::Function*);
	void evalNonExternalCall(const tpa::DefUseInstruction*);

	void evalMemcpy(const tpa::DefUseInstruction* duInst);
public:
	TrackingTransferFunction(const TaintGlobalState&, const tpa::Context*, ValueSet&, MemorySet&);

	void eval(const tpa::DefUseInstruction*);
};

}
}

#endif
