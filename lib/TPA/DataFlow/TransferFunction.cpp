#include "MemoryModel/Memory/MemoryManager.h"
#include "MemoryModel/Pointer/PointerManager.h"
#include "MemoryModel/Precision/ProgramLocation.h"
#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/PtsSet/StoreManager.h"
#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "PointerAnalysis/DataFlow/DefUseProgram.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "TPA/DataFlow/TransferFunction.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

static inline size_t countArguments(const Function* f)
{
	size_t ret = 0;
	for (auto& arg: f->args())
	{
		if (arg.getType()->isPointerTy())
			++ret;
	}
	return ret;
};

static PointerType* getMallocType(const Instruction* callInst)
{
	PointerType* mallocType = nullptr;
	unsigned numOfBitCastUses = 0;

	// Determine if CallInst has a bitcast use.
	for (auto user: callInst->users())
		if (const BitCastInst* bcInst = dyn_cast<BitCastInst>(user))
		{
			mallocType = cast<PointerType>(bcInst->getDestTy());
			numOfBitCastUses++;
		}

	// Malloc call has 1 bitcast use, so type is the bitcast's destination type.
	if (numOfBitCastUses == 1)
		return mallocType;

	// Malloc call was not bitcast, so type is the malloc function's return type.
	if (numOfBitCastUses == 0)
		return cast<PointerType>(callInst->getType());

	// Type could not be determined.
	return nullptr;
}

template <typename GraphType>
bool TransferFunction<GraphType>::evalAlloc(const Context* ctx, const AllocNodeMixin<NodeType>* allocNode, Env& env)
{
	auto allocInst = allocNode->getDest();
	assert(isa<AllocaInst>(allocInst));
	auto ptr = ptrManager.getOrCreatePointer(ctx, allocInst);

	auto memObj = memManager.allocateMemory(ProgramLocation(ctx, allocInst), allocNode->getAllocType());
	auto memLoc = memManager.offsetMemory(memObj, 0);

	return env.insertBinding(ptr, memLoc);
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalCopy(const Pointer* dst, const Pointer* src, Env& env)
{
	auto pSet = env.lookup(src);
	if (pSet == nullptr)
		return std::make_pair(false, false);

	auto changed = env.updateBinding(dst, pSet);
	return std::make_pair(true, changed);
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalCopy(const Context* ctx, const CopyNodeMixin<NodeType>* copyNode, Env& env)
{
	auto& pSetManager = storeManager.getPtsSetManager();
	auto resSet = pSetManager.getEmptySet();
	auto offset = copyNode->getOffset();

	auto dstPtr = ptrManager.getOrCreatePointer(ctx, copyNode->getDest());

	if (copyNode->getOffset() == 0)
	{
		// Single/Multiple srcs, no pointer arithmetic
		for (auto src: *copyNode)
		{
			auto srcPtr = ptrManager.getPointer(ctx, src);
			if (srcPtr == nullptr)
				return std::make_pair(false, false);

			auto pSet = env.lookup(srcPtr);
			if (pSet == nullptr)
				return std::make_pair(false, false);

			resSet = pSetManager.mergeSet(resSet, pSet);
		}
	}
	else
	{
		// Single src, pointer arithmetic
		assert(copyNode->getNumSrc() == 1 && "non-0-offset copy node must have only one src!");

		auto srcPtr = ptrManager.getPointer(ctx, copyNode->getFirstSrc());
		if (srcPtr == nullptr)
			return std::make_pair(false, false);

		auto pSet = env.lookup(srcPtr);
		if (pSet == nullptr)
			return std::make_pair(false, false);

		// Examine this set one-by-one
		for (auto tgtLoc: *pSet)
		{
			// We have two cases here:
			// - For non-array reference, just access the variable with the given offset
			// - For array reference, we have to examine the variables with offset * 0, offset * 1, offset * 2... all the way till the memory region boundary, if the memory object is not known to be an array previously (this may happen if the program contains nonarray-to-array bitcast)
			if (copyNode->isArrayRef() && !tgtLoc->isSummaryLocation())
			{
				auto objSize = tgtLoc->getMemoryObject()->getSize();

				for (unsigned i = 0, e = objSize - tgtLoc->getOffset(); i < e; i += offset)
				{
					auto obj = memManager.offsetMemory(tgtLoc, i);
					resSet = pSetManager.insert(resSet, obj);
				}
			}
			else
			{
				auto offsetLoc = memManager.offsetMemory(tgtLoc, offset);
				resSet = pSetManager.insert(resSet, offsetLoc);
			}
		}
	}

	// Perform strong update
	bool changed = env.updateBinding(dstPtr, resSet);
	return std::make_pair(true, changed);
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalLoad(const Context* ctx, const LoadNodeMixin<NodeType>* loadNode, Env& env, const Store& store)
{
	auto srcPtr = ptrManager.getPointer(ctx, loadNode->getSrc());
	assert(srcPtr != nullptr);
	auto dstPtr = ptrManager.getOrCreatePointer(ctx, loadNode->getDest());

	auto srcSet = env.lookup(srcPtr);
	if (srcSet == nullptr)
		return std::make_pair(false, false);

	auto& pSetManager = storeManager.getPtsSetManager();
	auto resSet = pSetManager.getEmptySet();

	bool isValid = false;
	for (auto tgtLoc: *srcSet)
	{
		if (auto tgtSet = store.lookup(tgtLoc))
		{
			isValid = true;
			resSet = pSetManager.mergeSet(resSet, tgtSet);
		}
	}

	if (!isValid)
		return std::make_pair(false, false);

	auto changed = env.updateBinding(dstPtr, resSet);

	return std::make_pair(true, changed);
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalStore(const Pointer* dstPtr, const Pointer* srcPtr, const Env& env, Store& store)
{
	auto srcSet = env.lookup(srcPtr);
	if (srcSet == nullptr)
		return std::make_pair(false, false);
	auto dstSet = env.lookup(dstPtr);
	if (dstSet == nullptr)
		return std::make_pair(false, false);

	assert(!dstSet->isEmpty() && "dstSet should not be empty");
	auto dstLoc = *dstSet->begin();
	if (dstSet->getSize() == 1 && dstLoc != memManager.getUniversalLocation() && !dstLoc->isSummaryLocation())
	{
		// Strong update
		auto changed = storeManager.strongUpdate(store, dstLoc, srcSet);
		return std::make_pair(true, changed);
	}

	// Weak update
	bool changed = false;
	for (auto updateLoc: *dstSet)
	{
		if (updateLoc == memManager.getUniversalLocation())
		{
			// We won't process stores into unknown location. This is an kind of unsound behavior that must be reported to the user
			//if (UniversalObjectWarning)
			//	errs() << "Warning: storing into a memory location that is untracked by the analysis.\n";
			continue;
		}
		changed |= storeManager.weakUpdate(store, updateLoc, srcSet);
	}
	return std::make_pair(true, changed);
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalStore(const Context* ctx, const StoreNodeMixin<NodeType>* storeNode, const Env& env, Store& store)
{
	auto srcPtr = ptrManager.getPointer(ctx, storeNode->getSrc());
	auto dstPtr = ptrManager.getPointer(ctx, storeNode->getDest());
	if (srcPtr == nullptr || dstPtr == nullptr)
		return std::make_pair(false, false);

	return evalStore(dstPtr, srcPtr, env, store);
}

template <typename GraphType>
bool TransferFunction<GraphType>::evalMalloc(const Pointer* dst, const llvm::Instruction* inst, const llvm::Value* sizeVal, Env& env, Store& store)
{
	auto mallocType = getMallocType(inst);
	if (mallocType == nullptr)
		mallocType = Type::getInt8PtrTy(inst->getContext());

	auto mallocSize = 0u;
	if (auto cInt = dyn_cast<ConstantInt>(sizeVal))
		mallocSize = cInt->getZExtValue() / (memManager.getDataLayout().getTypeAllocSize(mallocType));

	const MemoryObject* memObj = nullptr;
	if (mallocSize != 0)
		memObj = memManager.allocateMemory(ProgramLocation(dst->getContext(), inst), ArrayType::get(mallocType->getPointerElementType(), mallocSize));
	else
		memObj = memManager.allocateMemory(ProgramLocation(dst->getContext(), inst), mallocType->getPointerElementType(), true);

	auto memLoc = memManager.offsetMemory(memObj, 0);
	auto memSet = storeManager.getPtsSetManager().getSingletonSet(memLoc);
	return env.updateBinding(dst, memSet);
}

template <typename GraphType>
std::vector<const Function*> TransferFunction<GraphType>::resolveCallTarget(const Context* ctx, const CallNodeMixin<NodeType>* callNode, const Env& env, const llvm::ArrayRef<const Function*> defaultTargets)
{
	auto ret = std::vector<const Function*>();

	if (auto funPtr = ptrManager.getPointer(ctx, callNode->getFunctionPointer()))
	{
		if (auto funSet = env.lookup(funPtr))
		{
			// Guess addr-taken functions based on #params
			if (funSet->has(memManager.getUniversalLocation()))
			{
				for (auto tgtFunc: defaultTargets)
				{
					bool isArgMatch = tgtFunc->isVarArg() || (countArguments(tgtFunc) == callNode->getNumArgument());
					bool isRetMatch = ((tgtFunc->getReturnType()->isVoidTy()) == (callNode->getDest() == nullptr));
					if (isArgMatch && isRetMatch)
						ret.push_back(tgtFunc);
				}
			}
			else
			{
				for (auto funLoc: *funSet)
				{
					if (auto tgtFunc = memManager.getFunctionForObject(funLoc->getMemoryObject()))
						ret.push_back(tgtFunc);
				}
			}
		}
	}

	return ret;
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalCall(const Context* ctx, const CallNodeMixin<NodeType>* callNode, const Context* newCtx, const GraphType& tgtCFG, Env& env)
{
	// Handle arguments
	// Some codes do weird things like casting a 0-ary function into a vararg function and call it. We couldn't make any strict equality assumption about the number of arguments here except that the caller must provide enough actuals to make all the callee's formals well-defined
	assert(callNode->getNumArgument() <= countArguments(tgtCFG.getFunction()));

	auto argItr = callNode->begin();
	auto paramItr = tgtCFG.param_begin();
	// We must wait until all arugments are ready before proceed
	bool changed = false;
	while (argItr != callNode->end() && paramItr != tgtCFG.param_end())
	{
		if (!paramItr->getType()->isPointerTy())
		{
			++paramItr;
			continue;
		}

		auto argPtr = ptrManager.getPointer(ctx, *argItr);
		if (argPtr == nullptr)
			return std::make_pair(false, false);
		auto argSet = env.lookup(argPtr);
		if (argSet == nullptr)
			return std::make_pair(false, false);
		auto paramPtr = ptrManager.getOrCreatePointer(newCtx, paramItr);

		changed |= env.mergeBinding(paramPtr, argSet);
		
		++argItr;
		++paramItr;
	}

	return std::make_pair(true, changed);
}

template <typename GraphType>
std::pair<bool, bool> TransferFunction<GraphType>::evalReturn(const Context* newCtx, const ReturnNodeMixin<NodeType>* retNode, const Context* oldCtx, const CallNodeMixin<NodeType>* callNode, Env& env)
{
	auto retVal = retNode->getReturnValue();
	if (retVal == nullptr)
	{
		assert(callNode->getDest() == nullptr && "return an unexpected void");
		return std::make_pair(true, false);
	}

	auto retPtr = ptrManager.getPointer(newCtx, retVal);
	if (retPtr == nullptr)
		return std::make_pair(false, false);

	auto retSet = env.lookup(retPtr);
	if (retSet == nullptr)
		return std::make_pair(false, false);

	auto dstVal = callNode->getDest();
	if (dstVal == nullptr)
		return std::make_pair(true, false);

	auto dstPtr = ptrManager.getOrCreatePointer(oldCtx, dstVal);
	auto changed = env.mergeBinding(dstPtr, retSet);
	return std::make_pair(true, changed);
}

template <typename GraphType>
std::tuple<bool, bool, bool> TransferFunction<GraphType>::applyExternal(const Context* ctx, const CallNodeMixin<NodeType>* callNode, Env& env, Store& store, const llvm::Function* callee)
{
	auto extType = extTable.lookup(callee->getName());

	auto returnValue = [this, ctx, &env] (const Pointer* dstPtr, const Value* srcVal)
	{
		auto srcPtr = ptrManager.getPointer(ctx, srcVal);
		if (srcPtr == nullptr)
			return std::make_tuple(true, false, false);
		auto copyRet = evalCopy(dstPtr, srcPtr, env);
		return std::make_tuple(copyRet.first, copyRet.second, false);
	};

	// Local helper function to update the pts-sets of the memory locations in destSet with the pts-set of srcObj. If srcObj represents a larger memory chunk that includes multiple pointers, this function will automatically handle the pointer arithmetics
	auto ptsSetCopy = [this, &env, &store] (auto dstSet, const MemoryLocation* loc)
	{
		// If srcObj is larger than 1 pointer, then we need to copy all the memory contents as well
		auto locs = memManager.getAllOffsetLocations(loc);
		auto startingOffset = loc->getOffset();
		bool changed = false;
		for (auto oLoc: locs)
		{
			auto oSet = store.lookup(oLoc);
			if (oSet == nullptr)
				continue;

			auto offset = oLoc->getOffset() - startingOffset;
			for (auto updateLoc: *dstSet)
			{
				auto tgtLoc = memManager.offsetMemory(updateLoc, offset);
				if (tgtLoc == memManager.getUniversalLocation())
					break;
				changed |= storeManager.weakUpdate(store, tgtLoc, oSet);
			}
		}
		return changed;
	};

	auto fillNull = [this, &store] (const MemoryLocation* loc, Type* memType)
	{
		std::vector<const MemoryLocation*> candidates;

		while (auto arrayType = dyn_cast<ArrayType>(memType))
			memType = arrayType->getElementType();

		if (memType->isPointerTy())
			candidates.push_back(loc);
		else if (auto stType = dyn_cast<StructType>(memType))
		{
			std::function<void(StructType*, unsigned)> findCandidate = [this, &candidates, loc, &findCandidate] (StructType* stType, size_t baseOffset)
			{
				auto stLayout = memManager.getDataLayout().getStructLayout(stType);
				for (unsigned i = 0, e = stType->getNumElements(); i != e; ++i)
				{
					auto elemType = stType->getElementType(i);
					unsigned offset = stLayout->getElementOffset(i);

					while (elemType->isArrayTy())
						elemType = elemType->getArrayElementType();

					if (elemType->isPointerTy())
					{
						auto tgtLoc = memManager.offsetMemory(loc, baseOffset + offset);
						if (tgtLoc != memManager.getUniversalLocation())
							candidates.push_back(tgtLoc);
					}
					else if (auto subStType = dyn_cast<StructType>(elemType))
						findCandidate(subStType, baseOffset + offset);
				}
			};
			
			findCandidate(stType, 0);
		}

		bool changed = false;
		auto& pSetManager = storeManager.getPtsSetManager();
		auto nullSet = pSetManager.insert(pSetManager.getEmptySet(), memManager.getNullLocation());
		for (auto tgtObj: candidates)
		{
			changed |= storeManager.weakUpdate(store, tgtObj, nullSet);
		}

		return changed;
	};

	switch (extType)
	{
		case PointerEffect::NoEffect:
		{
			// Do nothing
			return std::make_tuple(true, false, false);
		}
		case PointerEffect::Realloc:
		{
			ImmutableCallSite cs(callNode->getInstruction());

			if (callNode->getDest() == nullptr)
				return std::make_tuple(true, false, false);
			auto dstPtr = ptrManager.getOrCreatePointer(ctx, callNode->getDest());

			if (!isa<ConstantPointerNull>(cs.getArgument(0)))
			{
				// Same as ReturnArg0
				return returnValue(dstPtr, cs.getArgument(0));
			}
			
			if (auto constSize = dyn_cast<Constant>(cs.getArgument(1)))
			{
				// Same as free()
				if (constSize->isNullValue())
					return std::make_tuple(true, false, false);
			}

			// else same as malloc case
			auto changed = evalMalloc(dstPtr, callNode->getInstruction(), cs.getArgument(1), env, store);
			return std::make_tuple(true, changed, false);
		}
		case PointerEffect::Malloc:
		{
			ImmutableCallSite cs(callNode->getInstruction());

			// If the return value is not used, don't bother process it
			if (callNode->getDest() == nullptr)
				return std::make_tuple(true, false, false);
			auto dstPtr = ptrManager.getOrCreatePointer(ctx, callNode->getDest());

			auto changed = evalMalloc(dstPtr, callNode->getInstruction(), cs.getArgument(0), env, store);
			return std::make_tuple(true, changed, false);
		}
		case PointerEffect::ReturnArg0:
		{
			assert(callNode->getNumArgument() >= 1);

			// If the return value is not used, don't bother process it
			if (callNode->getDest() == nullptr)
				return std::make_tuple(true, false, false);
			auto dstPtr = ptrManager.getOrCreatePointer(ctx, callNode->getDest());

			return returnValue(dstPtr, callNode->getArgument(0));
		}
		case PointerEffect::ReturnArg1:
		{
			assert(callNode->getNumArgument() >= 2);
			
			// If the return value is not used, don't bother process it
			if (callNode->getDest() == nullptr)
				return std::make_tuple(true, false, false);
			auto dstPtr = ptrManager.getOrCreatePointer(ctx, callNode->getDest());

			return returnValue(dstPtr, callNode->getArgument(1));
		}
		case PointerEffect::ReturnArg2:
		{
			assert(callNode->getNumArgument() >= 3);
			
			// If the return value is not used, don't bother process it
			if (callNode->getDest() == nullptr)
				return std::make_tuple(true, false, false);
			auto dstPtr = ptrManager.getOrCreatePointer(ctx, callNode->getDest());

			return returnValue(dstPtr, callNode->getArgument(2));
		}
		case PointerEffect::ReturnStatic:
		{
			// We can, of course, create a special variable which represents all the static memory locations
			// For now, however, just return a universal pointer
			// If the return value is not used, don't bother process it
			if (callNode->getDest() == nullptr)
				return std::make_tuple(true, false, false);
			auto dstPtr = ptrManager.getOrCreatePointer(ctx, callNode->getDest());
			
			auto copyRet = evalCopy(dstPtr, ptrManager.getUniversalPtr(), env);
			return std::make_tuple(copyRet.first, copyRet.second, false);
		}
		case PointerEffect::StoreArg0ToArg1:
		{
			assert(callNode->getNumArgument() >= 2);

			auto dstPtr = ptrManager.getPointer(ctx, callNode->getArgument(1));
			if (dstPtr == nullptr)
				return std::make_tuple(false, false, false);
			auto srcPtr = ptrManager.getPointer(ctx, callNode->getArgument(0));
			if (srcPtr == nullptr)
				return std::make_tuple(false, false, false);

			auto storeRet = evalStore(dstPtr, srcPtr, env, store);
			return std::make_tuple(storeRet.first, false, storeRet.second);
		}
		case PointerEffect::MemcpyArg1ToArg0:
		{
			// We assume arg0 is the dest, arg1 is the src
			assert(callNode->getNumArgument() >= 2);

			auto dstPtr = ptrManager.getPointer(ctx, callNode->getArgument(0));
			if (dstPtr == nullptr)
				return std::make_tuple(false, false, false);
			auto srcPtr = ptrManager.getPointer(ctx, callNode->getArgument(1));
			if (srcPtr == nullptr)
				return std::make_tuple(false, false, false);

			auto dstSet = env.lookup(dstPtr);
			if (dstSet == nullptr)
				return std::make_tuple(false, false, false);
			auto srcSet = env.lookup(srcPtr);
			if (srcSet == nullptr)
				return std::make_tuple(false, false, false);

			bool storeChanged = false;
			for (auto tgtLoc: *srcSet)
				storeChanged |= ptsSetCopy(dstSet, tgtLoc);

			// Process the return value
			bool envChanged = false;
			if (auto retVal = callNode->getDest())
			{
				auto retPtr = ptrManager.getOrCreatePointer(ctx, retVal);
				envChanged |= env.updateBinding(retPtr, dstSet);
			}
			return std::make_tuple(true, envChanged, storeChanged);
		}
		case PointerEffect::Memset:
		{
			assert(callNode->getNumArgument() >= 1);

			auto dstPtr = ptrManager.getPointer(ctx, callNode->getArgument(0));
			if (dstPtr == nullptr)
				return std::make_tuple(false, false, false);
			auto dstSet = env.lookup(dstPtr);
			if (dstSet == nullptr)
				return std::make_tuple(false, false, false);

			ImmutableCallSite cs(callNode->getInstruction());
			assert(cs && cs.arg_size() >= 2);
			bool isNull = false;
			if (auto cInt = dyn_cast<ConstantInt>(cs.getArgument(1)))
				isNull = cInt->isZero();

			// If we memset a pointer to zero, then all the memory locations has to be updated to NULL; otherwise, we treat the memory as a non-pointer constant area
			if (!isNull)
				return std::make_tuple(true, false, false);

			auto ptrType = cs.getArgument(0)->stripPointerCasts()->getType();
			assert(ptrType->isPointerTy());
			auto elemType = ptrType->getPointerElementType();
			if (!isa<SequentialType>(elemType) && !elemType->isStructTy())
				return std::make_tuple(true, false, false);

			bool storeChanged = false;
			for (auto tgtLoc: *dstSet)
			{
				if (tgtLoc == memManager.getUniversalLocation())
					continue;

				storeChanged |= fillNull(tgtLoc, elemType);
			}

			// Process the return value
			bool envChanged = false;
			if (auto retVal = callNode->getDest())
			{
				auto retPtr = ptrManager.getOrCreatePointer(ctx, retVal);
				envChanged |= env.updateBinding(retPtr, dstSet);
			}
			return std::make_tuple(true, envChanged, storeChanged);
		}
		default:
			llvm_unreachable("Unhandled external call");
	}
}

// explicit instantiations
template class TransferFunction<PointerCFG>;
template class TransferFunction<DefUseGraph>;

}
