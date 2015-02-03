#ifndef TPA_STATIC_CALL_GRAPH_H
#define TPA_STATIC_CALL_GRAPH_H

#include "Utils/VectorSet.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/iterator_range.h>

namespace llvm
{
	class Function;
	class Instruction;
}

namespace tpa
{

class Context;
class CallNode;
class PointerCFG;

class StaticCallGraph
{
public:
	using CallSiteType = std::pair<const Context*, const llvm::Instruction*>;
	using CallTargetType = std::pair<const Context*, const llvm::Function*>;
private:
	using CallSiteVector = SortedVectorSet<CallSiteType>;
	using CallTargetVector = SortedVectorSet<CallTargetType>;
	llvm::DenseMap<CallSiteType, CallTargetVector> callMap;
	llvm::DenseMap<CallTargetType, CallSiteVector> revCallMap;

	using CallSiteVectorConstIterator = CallSiteVector::const_iterator;
	using CallTargetVectorConstIterator = CallTargetVector::const_iterator;
public:
	StaticCallGraph() {}

	bool insertCallEdge(CallSiteType callSite, CallTargetType callTgt);

	llvm::iterator_range<CallTargetVectorConstIterator> getCallTargets(CallSiteType callSite) const;
	llvm::iterator_range<CallSiteVectorConstIterator> getCallSites(CallTargetType callTgt) const;
};

}

#endif
