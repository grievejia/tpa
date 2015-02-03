#ifndef TPA_MEMORY_MANAGER_H
#define TPA_MEMORY_MANAGER_H

#include "Memory/Memory.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Type.h>

#include <unordered_map>

namespace tpa
{

class MemoryManager
{
private:
	std::unordered_map<ProgramLocation, MemoryObject> objPool;
	std::unordered_set<MemoryLocation> locSet;

	MemoryObject universalObj;
	MemoryObject nullObj;
	const MemoryLocation* universalLoc;
	const MemoryLocation* nullLoc;

	llvm::DenseMap<const MemoryObject*, const llvm::Function*> objToFunction;

	llvm::DataLayout& dataLayout;

	const MemoryLocation* getMemoryLocation(const MemoryObject*, size_t offset, bool summary = false);
	size_t initializeMemoryObject(MemoryObject& obj, size_t offset, llvm::Type* t);
	size_t initializeSummaryObject(MemoryObject& obj, llvm::Type* t);
public:
	MemoryManager(llvm::DataLayout& d);

	const MemoryObject* getUniversalObject() const { return &universalObj; }
	const MemoryLocation* getUniversalLocation() { return universalLoc; }
	const MemoryObject* getNullObject() const { return &nullObj; }
	const MemoryLocation* getNullLocation() { return nullLoc; }
	const llvm::DataLayout& getDataLayout() const { return dataLayout; }

	const MemoryObject* allocateMemory(const ProgramLocation& loc, llvm::Type* type, bool forceSummary = false);
	const MemoryLocation* offsetMemory(const MemoryObject* obj, size_t offset);
	const MemoryLocation* offsetMemory(const MemoryLocation* loc, size_t offset);
	// Return all the MemoryLocations that share the same MemoryObject as loc
	std::vector<const MemoryLocation*> getAllOffsetLocations(const MemoryLocation* loc);

	const MemoryObject* createMemoryObjectForFunction(const llvm::Function* f);
	const llvm::Function* getFunctionForObject(const MemoryObject* obj) const;
};

}

#endif
