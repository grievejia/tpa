#pragma once

#include "PointerAnalysis/MemoryModel/AllocSite.h"
#include "PointerAnalysis/MemoryModel/MemoryBlock.h"
#include "PointerAnalysis/MemoryModel/MemoryObject.h"

#include <set>
#include <unordered_map>

namespace tpa
{

class MemoryManager
{
private:
	using AllocMap = std::unordered_map<AllocSite, MemoryBlock>;
	AllocMap allocMap;

	// Size of a pointer value
	size_t ptrSize;

	// Use the slow std::set here because we want the ordering
	mutable std::set<MemoryObject> objSet;

	// uBlock is the memory block representing the location that may points to anywhere. It is of the type byte array
	static const MemoryBlock uBlock;
	// nBlock is the memory block representing the location that must be null pointer. Its size is set to zero
	static const MemoryBlock nBlock;
	static const MemoryObject uObj;
	static const MemoryObject nObj;

	const MemoryObject* argvObj;
	const MemoryObject* envpObj;

	const MemoryBlock* allocateMemoryBlock(AllocSite, const TypeLayout*);
	const MemoryObject* getMemoryObject(const MemoryBlock*, size_t, bool) const;

	const MemoryObject* offsetMemory(const MemoryBlock*, size_t) const;
public:
	MemoryManager(size_t pSize = 8u);

	static const MemoryObject* getUniversalObject() { return &uObj; }
	static const MemoryObject* getNullObject() { return &nObj; }
	size_t getPointerSize() const { return ptrSize; }

	const MemoryObject* allocateGlobalMemory(const llvm::GlobalVariable*, const TypeLayout*);
	const MemoryObject* allocateMemoryForFunction(const llvm::Function* f);
	const MemoryObject* allocateStackMemory(const context::Context*, const llvm::Value*, const TypeLayout*);
	const MemoryObject* allocateHeapMemory(const context::Context*, const llvm::Value*, const TypeLayout*);

	const MemoryObject* allocateArgv(const llvm::Value*);
	const MemoryObject* allocateEnvp(const llvm::Value*);
	const MemoryObject* getArgvObject() const
	{
		assert(argvObj != nullptr);
		return argvObj;
	}
	const MemoryObject* getEnvpObject() const
	{
		assert(envpObj != nullptr);
		return envpObj;
	}

	const MemoryObject* offsetMemory(const MemoryObject*, size_t) const;
	// Return all MemoryObjects that share the same MemoryBlock as obj
	std::vector<const MemoryObject*> getReachableMemoryObjects(const MemoryObject*) const;
	// Return all MemoryObjects that might be pointer and share the same MemoryBlock as obj
	std::vector<const MemoryObject*> getReachablePointerObjects(const MemoryObject*, bool includeSelf = true) const;
};

}