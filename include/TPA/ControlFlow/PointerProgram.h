#ifndef TPA_POINTER_PROGRAM_H
#define TPA_POINTER_PROGRAM_H

#include "TPA/ControlFlow/PointerCFG.h"

#include <llvm/ADT/iterator_range.h>

#include <unordered_map>

namespace tpa
{

// A collection of mapping from llvm::Function to PointerCFG
class PointerProgram
{
private:
	using MapType = std::unordered_map<const llvm::Function*, PointerCFG>;
	MapType cfgMap;
	const PointerCFG* entryCFG;

	using FunctionListType = std::vector<const llvm::Function*>;
	FunctionListType addrTakenFunctions;
public:

	class const_iterator
	{
	private:
		MapType::const_iterator itr;

		using value_type = const typename MapType::const_iterator::value_type::second_type;
		using pointer = value_type*;
		using reference = value_type&;
	public:
		const_iterator(const MapType::const_iterator& i): itr(i) {}
		reference operator* () { return itr->second; }
		pointer operator->() { return &itr->second; }
		// Pre-increment
		const_iterator& operator++() { ++itr; return *this; }
		// Post-increment
		const_iterator operator++(int)
		{
			auto ret = *this;
			++itr;
			return ret;
		}
	};

	using const_fun_iterator = FunctionListType::const_iterator;

	PointerProgram(): entryCFG(nullptr) {}

	PointerCFG* createPointerCFGForFunction(const llvm::Function* f);
	const PointerCFG* getPointerCFG(const llvm::Function* f) const;
	PointerCFG* getPointerCFG(const llvm::Function* f);

	const PointerCFG* getEntryCFG() const
	{
		assert(entryCFG != nullptr && "Did not find entryCFG!");
		return entryCFG;
	}

	void writeDotFile(const llvm::StringRef& dirName, const llvm::StringRef& prefix) const;
	void writeDefUseDotFile(const llvm::StringRef& dirName, const llvm::StringRef& prefix) const;

	const_iterator begin() const { return const_iterator(cfgMap.begin()); }
	const_iterator end() const { return const_iterator(cfgMap.end()); }

	const_fun_iterator at_fun_begin() const { return addrTakenFunctions.begin(); }
	const_fun_iterator at_fun_end() const { return addrTakenFunctions.end(); }
	llvm::iterator_range<const_fun_iterator> at_funs() const
	{
		return llvm::iterator_range<const_fun_iterator>(at_fun_begin(), at_fun_end());
	}
};

}

#endif
