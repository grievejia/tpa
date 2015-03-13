#ifndef TPA_MEMORY_MANAGER_H
#define TPA_MEMORY_MANAGER_H

#include "MemoryModel/Memory/Memory.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Type.h>

#include <unordered_map>
#include <set>

namespace tpa
{

class MemoryManager
{
private:
	mutable std::unordered_map<ProgramLocation, MemoryObject> objPool;
	mutable std::set<MemoryLocation> locSet;

	MemoryObject universalObj;
	MemoryObject nullObj;
	const MemoryLocation* universalLoc;
	const MemoryLocation* nullLoc;
	const MemoryLocation* argvPtrLoc;
	const MemoryLocation* argvMemLoc;

	llvm::DenseMap<const MemoryObject*, const llvm::Function*> objToFunction;

	llvm::DataLayout& dataLayout;

	const MemoryLocation* getMemoryLocation(const MemoryObject*, size_t offset, bool summary = false) const;
	size_t initializeMemoryObject(MemoryObject& obj, size_t offset, llvm::Type* t);
	size_t initializeSummaryObject(MemoryObject& obj, llvm::Type* t);
public:
	MemoryManager(llvm::DataLayout& d);

	const MemoryObject* getUniversalObject() const { return &universalObj; }
	const MemoryLocation* getUniversalLocation() const { return universalLoc; }
	const MemoryObject* getNullObject() const { return &nullObj; }
	const MemoryLocation* getNullLocation() const { return nullLoc; }
	const llvm::DataLayout& getDataLayout() const { return dataLayout; }

	// FIXME: MemoryManager has to trust GlobalPointerAnalysis to set argv correctly. This is bad design
	void setArgv(const MemoryLocation* ptrLoc, const MemoryLocation* memLoc);
	const MemoryLocation* getArgvPtrLoc() const
	{
		assert(argvPtrLoc != nullptr);
		return argvPtrLoc;
	}
	const MemoryLocation* getArgvMemLoc() const
	{
		assert(argvMemLoc != nullptr);
		return argvMemLoc;
	}

	const MemoryObject* allocateMemory(const ProgramLocation& loc, llvm::Type* type, bool forceSummary = false);

	const MemoryLocation* offsetMemory(const MemoryObject* obj, size_t offset) const;
	const MemoryLocation* offsetMemory(const MemoryLocation* loc, size_t offset) const;
	
	// Return all the MemoryLocations that share the same MemoryObject as loc
	std::vector<const MemoryLocation*> getAllOffsetLocations(const MemoryLocation* loc) const;

	const MemoryObject* createMemoryObjectForFunction(const llvm::Function* f);
	const llvm::Function* getFunctionForObject(const MemoryObject* obj) const;
};

}

#endif
