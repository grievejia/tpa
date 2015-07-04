#pragma once

#include "PointerAnalysis/Engine/EvalSuccessor.h"
#include "PointerAnalysis/Support/Store.h"

#include <vector>

namespace tpa
{

class EvalResult
{
private:
	Store store;

	using SuccessorList = std::vector<EvalSuccessor>;
	SuccessorList succs;
public:
	using const_iterator = typename SuccessorList::const_iterator;

	EvalResult() = default;
	EvalResult(const Store* s)
	{
		if (s != nullptr)
			store = *s;
	}
	EvalResult(const EvalResult&) = delete;
	EvalResult(EvalResult&&) noexcept = default;
	EvalResult& operator=(const EvalResult&) = delete;
	EvalResult& operator=(EvalResult&&) = delete;

	Store& getStore() { return store; }
	const Store& getStore() const { return store; }
	void setStore(const Store& s) { store = s; }
	void setStore(Store&& s) { store = std::move(s); }

	void addTopLevelSuccessor(const ProgramPoint& pp)
	{
		succs.push_back(EvalSuccessor(pp, true));
	}

	void addMemLevelSuccessor(const ProgramPoint& pp)
	{
		succs.push_back(EvalSuccessor(pp, false));
	}

	const_iterator begin() const { return succs.begin(); }
	const_iterator end() const { return succs.end(); }
	bool empty() const { return succs.empty(); }
	size_t size() const { return succs.size(); }
};

}
