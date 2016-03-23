#pragma once

#include "PointerAnalysis/FrontEnd/Type/TypeMap.h"
#include "PointerAnalysis/Program/CFG/CFG.h"
#include "Util/Iterator/IteratorRange.h"
#include "Util/Iterator/MapValueIterator.h"

#include <unordered_map>
#include <vector>

namespace tpa
{

class SemiSparseProgram
{
private:
	const llvm::Module& module;
	TypeMap typeMap;

	using MapType = std::unordered_map<const llvm::Function*, CFG>;
	MapType cfgMap;

	using FunctionList = std::vector<const llvm::Function*>;
	FunctionList addrTakenFuncList;
public:
	using const_iterator = util::MapValueConstIterator<MapType::const_iterator>;
	using addr_taken_const_iterator = FunctionList::const_iterator;

	SemiSparseProgram(const llvm::Module& m);
	const llvm::Module& getModule() const { return module; }

	const TypeMap& getTypeMap() const { return typeMap; }
	void setTypeMap(TypeMap&& t) { typeMap = std::move(t); }

	CFG& getOrCreateCFGForFunction(const llvm::Function& f);
	const CFG* getCFGForFunction(const llvm::Function& f) const;
	const CFG* getEntryCFG() const;

	const_iterator begin() const { return const_iterator(cfgMap.begin()); }
	const_iterator end() const { return const_iterator(cfgMap.end()); }

	addr_taken_const_iterator addr_taken_func_begin() const { return addrTakenFuncList.begin(); }
	addr_taken_const_iterator addr_taken_func_end() const { return addrTakenFuncList.end(); }
	auto addr_taken_funcs() const
	{
		return util::iteratorRange(addrTakenFuncList.begin(), addrTakenFuncList.end());
	}
};

}