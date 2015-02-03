#ifndef TPA_POINTER_MANAGER_H
#define TPA_POINTER_MANAGER_H

#include "Memory/Pointer.h"

#include <unordered_map>

namespace tpa
{

// A class that is responsible for allocating variables and maintain the mapping between llvm values with TPA variables
class PointerManager
{
private:
	// Allocated pointers
	std::unordered_set<Pointer> ptrSet;

	// Special pointer that may point to any memory object
	Pointer universalPtr;
	// Special pointer that may point to no memory object
	Pointer nullPtr;

	// Group together Pointers with the same Value
	using PointerVector = std::vector<const Pointer*>;
	std::unordered_map<const llvm::Value*, PointerVector> valuePtrMap;
public:
	PointerManager();
	const Pointer* getUniversalPtr() const
	{
		return &universalPtr;
	}
	const Pointer* getNullPtr() const
	{
		return &nullPtr;
	}

	// Return a Pointer corresponds to (ctx, val). If not exist, create one
	const Pointer* getOrCreatePointer(const Context* ctx, const llvm::Value* val);
	// Return a Pointer corresponds to (ctx, val). If not exist, return NULL
	const Pointer* getPointer(const Context* ctx, const llvm::Value* val) const;
	// Return a vector of Pointers, whose elements corresponds to the same llvm::Value. Return NULL if not such Pointer is found
	PointerVector getPointersWithValue(const llvm::Value* val) const;
};

}

#endif
