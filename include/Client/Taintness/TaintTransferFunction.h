#ifndef INFOFLOW_TRANSFER_FUNCTION_H
#define INFOFLOW_TRANSFER_FUNCTION_H

#include "Client/Taintness/SourceSink.h"
#include "Client/Taintness/TaintEnvStore.h"
#include "PointerAnalysis/External/ExternalPointerEffectTable.h"
#include "Utils/Hashing.h"

#include <llvm/IR/CallSite.h>

#include <vector>
#include <unordered_set>

namespace llvm
{
	class Instruction;
}

namespace tpa
{
	class Context;
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class TaintTransferFunction
{
private:
	SourceSinkManager ssManager;

	const tpa::PointerAnalysis& ptrAnalysis;
	const tpa::ExternalPointerEffectTable& extTable;

	// Stash uloc and nloc here to grab them quickly during analysis
	const tpa::MemoryLocation *uLoc, *nLoc;

	struct SinkRecord
	{
		const tpa::Context* context;
		const llvm::Instruction* inst;
		const llvm::Function* callee;

		bool operator==(const SinkRecord& rhs) const
		{
			return (context == rhs.context) && (inst == rhs.inst) && (callee == rhs.callee);
		}
	};
	struct SinkRecordHasher
	{
		size_t operator()(const SinkRecord& s) const
		{
			size_t ret = 0;
			tpa::hash_combine(ret, s.context);
			tpa::hash_combine(ret, s.inst);
			tpa::hash_combine(ret, s.callee);
			return ret;
		}
	};
	std::unordered_set<SinkRecord, SinkRecordHasher> sinkPoints;

	// Return false if the check failed
	bool checkValue(const TSummary& summary, const tpa::Context*, llvm::ImmutableCallSite cs, const TaintEnv& env, const TaintStore& store);
	bool checkValue(const TEntry& entry, tpa::ProgramLocation pLoc, const TaintEnv& env, const TaintStore& store);
public:
	TaintTransferFunction(const tpa::PointerAnalysis& pa, const tpa::ExternalPointerEffectTable& e);

	std::tuple<bool, bool, bool> evalInst(const tpa::Context*, const llvm::Instruction*, TaintEnv&, TaintStore&);
	std::tuple<bool, bool, bool> processLibraryCall(const tpa::Context* ctx, const llvm::Function* callee, llvm::ImmutableCallSite cs, TaintEnv&, TaintStore&);
	bool checkMemoStates(const TaintEnv&, const std::unordered_map<tpa::ProgramLocation, TaintStore>&);
};

}	// end of taint
}	// end of client

#endif
