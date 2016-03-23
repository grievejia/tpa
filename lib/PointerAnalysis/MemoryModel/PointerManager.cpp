#include "Context/Context.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>

using namespace context;

namespace tpa
{

const llvm::Value* canonicalizeValue(const llvm::Value* value)
{
	assert(value != nullptr);
	value = value->stripPointerCasts();
	if (auto phiNode = llvm::dyn_cast<llvm::PHINode>(value))
	{
		const llvm::Value* rhs = nullptr;
		for (auto& op: phiNode->operands())
		{
			auto val = op.get()->stripPointerCasts();
			if (rhs == nullptr)
				rhs = val;
			else if (val != rhs)
			{
				rhs = nullptr;
				break;
			}
		}

		if (rhs != nullptr)
			value = rhs;
	}
	else if (llvm::isa<llvm::IntToPtrInst>(value))
		value = llvm::UndefValue::get(value->getType());

	return value;
}

PointerManager::PointerManager(): uPtr(nullptr), nPtr(nullptr) {}

const Pointer* PointerManager::buildPointer(const context::Context* ctx, const llvm::Value* val)
{
	auto ptr = Pointer(ctx, val);
	auto itr = ptrSet.find(ptr);
	if (itr != ptrSet.end())
		return &*itr;

	itr = ptrSet.insert(itr, ptr);
	auto ret = &*itr;
	valuePtrMap[val].push_back(ret);
	return ret;
}

const Pointer* PointerManager::setUniversalPointer(const llvm::UndefValue* v)
{
	assert(uPtr == nullptr);
	assert(v->getType() == llvm::Type::getInt8PtrTy(v->getContext()));
	uPtr = buildPointer(Context::getGlobalContext(), v);
	return uPtr;
}

const Pointer* PointerManager::getUniversalPointer() const
{
	assert(uPtr != nullptr);
	return uPtr;
}

const Pointer* PointerManager::setNullPointer(const llvm::ConstantPointerNull* v)
{
	assert(nPtr == nullptr);
	assert(v->getType() == llvm::Type::getInt8PtrTy(v->getContext()));
	nPtr = buildPointer(Context::getGlobalContext(), v);
	return nPtr;
}

const Pointer* PointerManager::getNullPointer() const
{
	assert(nPtr != nullptr);
	return nPtr;
}


const Pointer* PointerManager::getPointer(const Context* ctx, const llvm::Value* val) const
{
	assert(ctx != nullptr && val != nullptr);

	val = canonicalizeValue(val);
	
	if (llvm::isa<llvm::ConstantPointerNull>(val))
		return nPtr;
	else if (llvm::isa<llvm::UndefValue>(val))
		return uPtr;
	else if (llvm::isa<llvm::GlobalValue>(val))
		ctx = Context::getGlobalContext();

	auto itr = ptrSet.find(Pointer(ctx, val));
	if (itr == ptrSet.end())
		return nullptr;
	else
		return &*itr;
}

const Pointer* PointerManager::getOrCreatePointer(const Context* ctx, const llvm::Value* val)
{
	assert(ctx != nullptr && val != nullptr);

	val = canonicalizeValue(val);

	if (llvm::isa<llvm::ConstantPointerNull>(val))
		return nPtr;
	else if (llvm::isa<llvm::UndefValue>(val))
		return uPtr;
	else if (llvm::isa<llvm::GlobalValue>(val))
		ctx = Context::getGlobalContext();

	return buildPointer(ctx, val);
}

PointerManager::PointerVector PointerManager::getPointersWithValue(const llvm::Value* val) const
{
	PointerVector vec;

	val = canonicalizeValue(val);

	if (llvm::isa<llvm::ConstantPointerNull>(val))
		vec.push_back(nPtr);
	else if (llvm::isa<llvm::UndefValue>(val))
		vec.push_back(uPtr);
	else
	{
		auto itr = valuePtrMap.find(val);
		if (itr != valuePtrMap.end())
			vec = itr->second;
	}

	return vec;
}

}