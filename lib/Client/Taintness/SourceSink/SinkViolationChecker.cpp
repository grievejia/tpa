#include "Client/Taintness/DataFlow/TaintEnv.h"
#include "Client/Taintness/SourceSink/Checker/SinkSignature.h"
#include "Client/Taintness/SourceSink/Checker/SinkViolationChecker.h"
#include "Client/Taintness/SourceSink/Table/SourceSinkLookupTable.h"
#include "PointerAnalysis/Analysis/PointerAnalysis.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

SinkViolationChecker::SinkViolationChecker(const TaintEnv& e, const TaintStore& s, const SourceSinkLookupTable& t, const PointerAnalysis& p): env(e), store(s), table(t), ptrAnalysis(p)
{
}

TaintLattice SinkViolationChecker::lookupTaint(const tpa::ProgramLocation& pLoc, TClass what)
{
	switch (what)
	{
		case TClass::ValueOnly:
			return env.lookup(pLoc);
		case TClass::DirectMemory:
		{
			auto pSet = ptrAnalysis.getPtsSet(pLoc.getContext(), pLoc.getInstruction());
			assert(!pSet.isEmpty());
			auto res = TaintLattice::Unknown;
			for (auto loc: pSet)
			{
				auto val = store.lookup(loc);
				res = Lattice<TaintLattice>::merge(res, val);
			}
			return res;
		}
		case TClass::ReachableMemory:
			llvm_unreachable("ReachableMemory shouldn't appear in sink entry");
	}
}

void SinkViolationChecker::checkValueWithTClass(const ProgramLocation& pLoc, TClass tClass, uint8_t argPos, SinkViolationRecords& violations)
{
	auto currVal = lookupTaint(pLoc, tClass);
	auto cmpRes = Lattice<TaintLattice>::compare(TaintLattice::Untainted, currVal);

	if (cmpRes != LatticeCompareResult::Equal && cmpRes != LatticeCompareResult::GreaterThan)
		violations.push_back({ argPos, tClass, TaintLattice::Untainted, currVal });
}

void SinkViolationChecker::checkCallSiteWithEntry(const DefUseProgramLocation& pLoc, const SinkTaintEntry& entry, SinkViolationRecords& violations)
{
	auto taintPos = entry.getArgPosition().getAsArgPosition();

	ImmutableCallSite cs(pLoc.getDefUseInstruction()->getInstruction());
	auto checkArgument = [this, &pLoc, &entry, &violations, &cs] (size_t idx)
	{
		auto argPLoc = ProgramLocation(pLoc.getContext(), cs.getArgument(idx));
		checkValueWithTClass(argPLoc, entry.getTaintClass(), idx, violations);
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

SinkViolationRecords SinkViolationChecker::checkCallSiteWithSummary(const DefUseProgramLocation& pLoc, const TaintSummary& summary)
{
	SinkViolationRecords violations;
	ImmutableCallSite cs(pLoc.getDefUseInstruction()->getInstruction());

	for (auto const& entry: summary)
	{
		// We only care about the sink here
		if (entry.getEntryEnd() != TEnd::Sink)
			continue;

		checkCallSiteWithEntry(pLoc, entry.getAsSinkEntry(), violations);
	}

	return violations;
}

SinkViolationRecords SinkViolationChecker::checkSinkViolation(const SinkSignature& sig)
{
	SinkViolationRecords violations;
	
	if (auto taintSummary = table.getSummary(sig.getCallee()->getName()))
		violations = checkCallSiteWithSummary(sig.getCallSite(), *taintSummary);

	return violations;
}

}
}