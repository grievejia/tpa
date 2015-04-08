#ifndef TPA_TRANSFER_FUNCTION_H
#define TPA_TRANSFER_FUNCTION_H

#include "MemoryModel/PtsSet/Env.h"
#include "MemoryModel/PtsSet/Store.h"
#include "PointerAnalysis/ControlFlow/PointerCFG.h"
#include "TPA/DataFlow/EvalStatus.h"

#include <tuple>

namespace tpa
{

class Context;
class MemoryManager;
class PointerManager;

class TransferFunction
{
private:
	const Context* ctx;
	Store* store;

	SemiSparseGlobalState& globalState;

	// General helpers
	const Pointer* getPointer(const llvm::Value*);
	const Pointer* getOrCreatePointer(const llvm::Value*);
	const Pointer* getOrCreatePointer(const Context*, const llvm::Value*);

	// evalAlloc helpers
	EvalStatus evalMemoryAllocation(const Pointer*, llvm::Type*, bool);

	// evalCopy helpers
	EvalStatus evalCopyNodeWithZeroOffset(const CopyNode*);
	EvalStatus evalCopyNodeWithNonZeroOffset(const CopyNode*);
	EvalStatus copyWithOffset(const Pointer*, const Pointer*, size_t, bool);
	PtsSet updateOffsetLocation(PtsSet, const MemoryLocation*, size_t, bool);

	// evalStore helpers
	EvalStatus evalStore(const Pointer*, const Pointer*);
	EvalStatus strongUpdateStore(const MemoryLocation*, PtsSet);
	EvalStatus weakUpdateStore(PtsSet, PtsSet);

	// evalExternalCall helpers
	EvalStatus evalMalloc(const CallNode*);
	size_t getMallocSize(llvm::Type*, const llvm::Value*);
	EvalStatus evalMallocWithSizeValue(const CallNode*, const llvm::Value*);
	EvalStatus evalRealloc(const CallNode*);
	EvalStatus copyPointerPtsSet(const Pointer*, const Pointer*);
	EvalStatus evalExternalReturnsArg(const CallNode*, size_t);
	EvalStatus evalExternalReturnsStatic(const CallNode*);
	EvalStatus evalExternalStore(const CallNode*, size_t, size_t);
	EvalStatus evalMemcpy(const CallNode*, size_t, size_t);
	EvalStatus evalMemcpyPointer(const Pointer*, const Pointer*);
	bool copyMemoryPtsSet(const MemoryLocation*, const std::vector<const MemoryLocation*>&, size_t);
	EvalStatus evalMemset(const CallNode*);
	EvalStatus fillPtsSetWithNull(const Pointer*);
	bool setMemoryLocationToNull(const MemoryLocation*, llvm::Type*);
	std::vector<const MemoryLocation*> findPointerCandidates(const MemoryLocation*, llvm::Type*);
	std::vector<const MemoryLocation*> findPointerCandidatesInStruct(const MemoryLocation*, llvm::StructType*, size_t);

	// resolveCallTargets helpers
	std::vector<const llvm::Function*> findFunctionsInPtsSet(PtsSet, const CallNode*);

	// evalCallArgumets helpers
	std::vector<PtsSet> collectArgumentPtsSets(const CallNode*, size_t);
	bool updateParameterPtsSets(const Context*, const PointerCFG&, const std::vector<PtsSet>&);
public:
	TransferFunction(const Context*, SemiSparseGlobalState&);
	TransferFunction(const Context*, Store&, SemiSparseGlobalState&);

	EvalStatus evalAllocNode(const AllocNode*);
	EvalStatus evalCopyNode(const CopyNode*);
	EvalStatus evalLoadNode(const LoadNode*);
	EvalStatus evalStoreNode(const StoreNode*);
	EvalStatus evalExternalCall(const CallNode*, const llvm::Function*);
	EvalStatus evalCallArguments(const CallNode*, const Context*, const llvm::Function*);
	EvalStatus evalReturnValue(const ReturnNode*, const Context*, const CallNode*);

	std::vector<const llvm::Function*> resolveCallTarget(const CallNode*);
};

}

#endif
