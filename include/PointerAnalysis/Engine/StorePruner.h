#pragma once

#include "PointerAnalysis/Support/ProgramPoint.h"
#include "PointerAnalysis/Support/Env.h"
#include "PointerAnalysis/Support/Store.h"
#include "PointerAnalysis/Support/FunctionContext.h"
#include "Util/DataStructure/VectorSet.h"

#include <unordered_map>
#include <unordered_set>

namespace tpa
{

class MemoryManager;
class PointerManager;

class StorePruner
{
private:
	const Env& env;
	const PointerManager& ptrManager;
	const MemoryManager& memManager;

	using MapType = std::unordered_map<FunctionContext, Store>;
	MapType prunedMap;

	using ObjectSet = std::unordered_set<const MemoryObject*>;
	ObjectSet getRootSet(const Store&, const ProgramPoint&);
	void findAllReachableObjects(const Store&, ObjectSet&);
	std::pair<Store, Store> splitStore(const Store&, const ObjectSet&);
public:
	StorePruner(const Env& e, const PointerManager& p, const MemoryManager& m): env(e), ptrManager(p), memManager(m) {}

	Store pruneStore(const Store&, const ProgramPoint&);
	const Store* lookupPrunedStore(const FunctionContext&);
};

}
