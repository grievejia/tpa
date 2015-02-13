#ifndef TPA_DEF_USE_PROGRAM_H
#define TPA_DEF_USE_PROGRAM_H

#include "PointerAnalysis/DataFlow/DefUseGraph.h"

namespace tpa
{

class DefUseProgram
{
private:
	using MapType = std::unordered_map<const llvm::Function*, DefUseGraph>;
	MapType dugMap;
	const DefUseGraph* entryGraph;
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
		bool operator==(const const_iterator& rhs) const
		{
			return itr == rhs.itr;
		}
		bool operator!=(const const_iterator& rhs) const
		{
			return itr != rhs.itr;
		}
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

	DefUseProgram(): entryGraph(nullptr) {}

	DefUseGraph* createDefUseGraphForFunction(const llvm::Function* f);
	const DefUseGraph* getDefUseGraph(const llvm::Function* f) const;
	DefUseGraph* getDefUseGraph(const llvm::Function* f);

	const DefUseGraph* getEntryGraph() const
	{
		assert(entryGraph != nullptr && "Did not find entryCFG!");
		return entryGraph;
	}

	void writeDotFile(const llvm::StringRef& dirName, const llvm::StringRef& prefix) const;

	const_iterator begin() const { return const_iterator(dugMap.begin()); }
	const_iterator end() const { return const_iterator(dugMap.end()); }
};

}

#endif
