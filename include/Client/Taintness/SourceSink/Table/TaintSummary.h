#ifndef TPA_TAINT_SUMMARY_H
#define TPA_TAINT_SUMMARY_H

#include "Client/Taintness/SourceSink/Table/TaintEntry.h"

#include <memory>
#include <vector>

namespace client
{
namespace taint
{

class TaintSummary
{
private:
	using DeclList = std::vector<TaintEntry>;
	DeclList effects;
public:
	using const_iterator = DeclList::const_iterator;

	TaintSummary() = default;

	void addEntry(TaintEntry entry)
	{
		effects.push_back(std::move(entry));
	}

	std::size_t size() const { return effects.size(); }
	bool empty() const { return effects.empty(); }

	const_iterator begin() const { return effects.begin(); }
	const_iterator end() const { return effects.end(); }
};

}
}

#endif
