#include "Memory/PointerManager.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>

using namespace llvm;

namespace tpa
{

PointerManager::PointerManager(): universalPtr(Context::getGlobalContext(), nullptr), nullPtr(Context::getGlobalContext(), nullptr)
{
}

const Pointer* PointerManager::getPointer(const Context* ctx, const llvm::Value* val) const
{
	assert(ctx != nullptr && val != nullptr && val->getType()->isPointerTy());

	if (isa<ConstantPointerNull>(val))
		return &nullPtr;

	auto ptr = Pointer(ctx, val);
	auto itr = ptrSet.find(ptr);
	if (itr == ptrSet.end())
		return nullptr;
	else
		return &*itr;
}

const Pointer* PointerManager::getOrCreatePointer(const Context* ctx, const llvm::Value* val)
{
	assert(ctx != nullptr && val != nullptr && val->getType()->isPointerTy());

	if (isa<ConstantPointerNull>(val))
		return &nullPtr;

	auto ptr = Pointer(ctx, val);
	auto itr = ptrSet.find(ptr);
	if (itr != ptrSet.end())
		return &*itr;

	itr = ptrSet.insert(itr, ptr);
	auto ret = &*itr;
	valuePtrMap[val].push_back(ret);
	return ret;
}

PointerManager::PointerVector PointerManager::getPointersWithValue(const llvm::Value* val) const
{
	auto itr = valuePtrMap.find(val);
	if (itr == valuePtrMap.end())
		return PointerVector();
	else
		return PointerVector(itr->second);
}

}
