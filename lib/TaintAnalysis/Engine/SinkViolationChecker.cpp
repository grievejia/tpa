#include "Annotation/Taint/ExternalTaintTable.h"
#include "PointerAnalysis/Analysis/SemiSparsePointerAnalysis.h"
#include "TaintAnalysis/Engine/SinkViolationChecker.h"
#include "TaintAnalysis/Program/DefUseInstruction.h"
#include "TaintAnalysis/Support/SinkSignature.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintMemo.h"
#include "TaintAnalysis/Support/TaintValue.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;

namespace taint
{

TaintLattice SinkViolationChecker::lookupTaint(const TaintValue& tv, TClass what, const TaintStore* store)
{
	switch (what)
	{
		case TClass::ValueOnly:
			return env.lookup(tv);
		case TClass::DirectMemory:
		{
			assert(store != nullptr);
			auto pSet = ptrAnalysis.getPtsSet(tv.getContext(), tv.getValue());
			assert(!pSet.empty());
			auto res = TaintLattice::Unknown;
			for (auto loc: pSet)
			{
				auto val = store->lookup(loc);
				res = Lattice<TaintLattice>::merge(res, val);
			}
			return res;
		}
		case TClass::ReachableMemory:
			llvm_unreachable("ReachableMemory shouldn't appear in sink entry");
	}
}

void SinkViolationChecker::checkValueWithTClass(const TaintValue& tv, TClass tClass, uint8_t argPos, const TaintStore* store, SinkViolationList& violations)
{
	auto currVal = lookupTaint(tv, tClass, store);
	auto cmpRes = Lattice<TaintLattice>::compare(TaintLattice::Untainted, currVal);

	if (cmpRes != LatticeCompareResult::Equal && cmpRes != LatticeCompareResult::GreaterThan)
		violations.push_back({ argPos, tClass, TaintLattice::Untainted, currVal });
}

void SinkViolationChecker::checkCallSiteWithEntry(const ProgramPoint& pp, const SinkTaintEntry& entry, SinkViolationList& violations)
{
	auto taintPos = entry.getArgPosition().getAsArgPosition();

	ImmutableCallSite cs(pp.getDefUseInstruction()->getInstruction());
	auto checkArgument = [this, &pp, &entry, &violations, &cs] (size_t idx)
	{
		auto store = memo.lookup(pp);

		auto argVal = TaintValue(pp.getContext(), cs.getArgument(idx));
		checkValueWithTClass(argVal, entry.getTaintClass(), idx, store, violations);
	};
	if (taintPos.isAfterArgPosition())
	{
		for (size_t i = taintPos.getArgIndex(), e = cs.arg_size(); i < e; ++i)
			checkArgument(i);
	}
	else
	{
		checkArgument(taintPos.getArgIndex());
	}

}

SinkViolationList SinkViolationChecker::checkCallSiteWithSummary(const ProgramPoint& pp, const TaintSummary& summary)
{
	SinkViolationList violations;
	ImmutableCallSite cs(pp.getDefUseInstruction()->getInstruction());

	for (auto const& entry: summary)
	{
		// We only care about the sink here
		if (entry.getEntryEnd() != TEnd::Sink)
			continue;

		checkCallSiteWithEntry(pp, entry.getAsSinkEntry(), violations);
	}

	return violations;
}

void SinkViolationChecker::checkSinkViolation(const SinkSignature& sig, SinkViolationRecord& records)
{
	if (auto taintSummary = table.lookup(sig.getCallee()->getName()))
	{
		auto callsite = sig.getCallSite();
		auto violations = checkCallSiteWithSummary(sig.getCallSite(), *taintSummary);
		if (!violations.empty())
			records[callsite] = std::move(violations);
	}
	else
		llvm_unreachable("Unrecognized external function call");
}

}