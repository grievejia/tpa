#pragma once

#include "TaintAnalysis/Program/DefUseFunction.h"
#include "Util/Iterator/MapValueIterator.h"

namespace llvm
{
	class Module;
}

namespace taint
{

class DefUseModule
{
private:
	const llvm::Module& module;
	using FunctionMap = std::unordered_map<const llvm::Function*, DefUseFunction>;
	FunctionMap funMap;
	const DefUseFunction* entryFunc;
public:
	using iterator = util::MapValueIterator<FunctionMap::iterator>;
	using const_iterator = util::MapValueConstIterator<FunctionMap::const_iterator>;

	DefUseModule(const llvm::Module& m);
	const llvm::Module& getModule() const { return module; }

	const DefUseFunction& getEntryFunction() const
	{
		assert(entryFunc != nullptr);
		return *entryFunc;
	};
	const DefUseFunction& getDefUseFunction(const llvm::Function* f) const;

	iterator begin() { return iterator(funMap.begin()); }
	iterator end() { return iterator(funMap.end()); }
	const_iterator begin() const { return const_iterator(funMap.begin()); }
	const_iterator end() const { return const_iterator(funMap.end()); }
};

}
