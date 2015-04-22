#ifndef TPA_TAINT_SUMMARY_H
#define TPA_TAINT_SUMMARY_H

#include "Client/Taintness/SourceSink/Table/TaintEntry.h"

#include <llvm/ADT/iterator.h>

#include <memory>
#include <vector>

namespace client
{
namespace taint
{

class TaintSummary
{
private:
	using DeclList = std::vector<std::unique_ptr<TaintEntry>>;
	DeclList effects;
public:
	struct const_iterator: public llvm::iterator_adaptor_base<const_iterator, DeclList::const_iterator>
	{
		explicit const_iterator(const DeclList::const_iterator& i): llvm::iterator_adaptor_base<const_iterator, DeclList::const_iterator>(i) {}
		const TaintEntry& operator*() const { return **I; }
		const TaintEntry& operator->() const { return operator*(); }
	};

	TaintSummary() = default;

	void addEntry(std::unique_ptr<TaintEntry> entry)
	{
		effects.push_back(std::move(entry));
	}

	std::size_t getSize() const { return effects.size(); }

	const_iterator begin() const { return const_iterator(effects.begin()); }
	const_iterator end() const { return const_iterator(effects.end()); }
};

}
}

#endif
