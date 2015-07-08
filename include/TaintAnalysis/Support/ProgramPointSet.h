#pragma once

#include "TaintAnalysis/Support/ProgramPoint.h"

#include <unordered_set>

namespace taint
{

class ProgramPointSet
{
private:
	using SetType = std::unordered_set<ProgramPoint>;
	SetType ppSet;
public:
	using const_iterator = SetType::const_iterator;

	ProgramPointSet() = default;

	ProgramPointSet(const ProgramPointSet&) = delete;
	ProgramPointSet(ProgramPointSet&&) noexcept = default;
	ProgramPointSet& operator=(const ProgramPointSet&) = delete;
	ProgramPointSet& operator=(ProgramPointSet&&) = delete;

	bool insert(const ProgramPoint& pp)
	{
		return ppSet.insert(pp).second;
	}

	const_iterator begin() const { return ppSet.begin(); }
	const_iterator end() const { return ppSet.end(); }
	size_t size() const { return ppSet.size(); }
	bool empty() const { return ppSet.empty(); }
};

}