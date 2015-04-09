#include "Client/Taintness/SourceSink/SinkViolationChecker.h"
#include "Client/Taintness/SourceSink/SourceSinkLookupTable.h"
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
	}
}

SinkViolationChecker::SinkViolationRecords SinkViolationChecker::checkCallSiteWithSummary(const ProgramLocation& pLoc, const TSummary& summary)
{
	SinkViolationRecords violations;

	auto ctx = pLoc.getContext();
	ImmutableCallSite cs(pLoc.getInstruction());

	for (auto const& entry: summary)
	{
		// We only care about the sink here
		if (entry.end == TEnd::Source)
			continue;

		auto checkProgramLocation = [this, &entry, &violations] (const ProgramLocation& pLoc)
		{
			auto currVal = lookupTaint(pLoc, entry.what);
			auto cmpRes = Lattice<TaintLattice>::compare(entry.val, currVal);
			if (cmpRes != LatticeCompareResult::Equal && cmpRes != LatticeCompareResult::GreaterThan)
			{
				violations.push_back({ entry.pos, entry.what, entry.val, currVal });
			}
		};

		switch (entry.pos)
		{
			case TPosition::Ret:
			{
				checkProgramLocation(pLoc);
				break;
			}
			case TPosition::Arg0:
			{
				checkProgramLocation(ProgramLocation(ctx, cs.getArgument(0)));
				break;
			}
			case TPosition::Arg1:
			{
				checkProgramLocation(ProgramLocation(ctx, cs.getArgument(1)));
				break;
			}
			case TPosition::Arg2:
			{
				checkProgramLocation(ProgramLocation(ctx, cs.getArgument(2)));
				break;
			}
			case TPosition::Arg3:
			{
				checkProgramLocation(ProgramLocation(ctx, cs.getArgument(3)));
				break;
			}
			case TPosition::Arg4:
			{
				checkProgramLocation(ProgramLocation(ctx, cs.getArgument(4)));
				break;
			}
			case TPosition::AfterArg0:
			{
				for (auto i = 1u, e = cs.arg_size(); i < e; ++i)
					checkProgramLocation(ProgramLocation(ctx, cs.getArgument(i)));
				break;
			}
			case TPosition::AfterArg1:
			{
				for (auto i = 2u, e = cs.arg_size(); i < e; ++i)
					checkProgramLocation(ProgramLocation(ctx, cs.getArgument(i)));
				break;
			}
			case TPosition::AllArgs:
			{
				for (auto i = 0u, e = cs.arg_size(); i < e; ++i)
					checkProgramLocation(ProgramLocation(ctx, cs.getArgument(i)));

				break;
			}
		}
	}

	return violations;
}

SinkViolationChecker::SinkViolationRecords SinkViolationChecker::checkSinkViolation(const ProgramLocation& pLoc, const Function* callee)
{
	SinkViolationRecords violations;
	
	if (auto taintSummary = table.getSummary(callee->getName()))
		violations = checkCallSiteWithSummary(pLoc, *taintSummary);

	return violations;
}

}
}