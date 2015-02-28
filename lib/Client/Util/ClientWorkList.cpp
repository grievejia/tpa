#include "Client/Util/ClientWorkList.h"

#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Module.h>

using namespace llvm;
using namespace tpa;

namespace client
{

BasicBlockPrioComparator::BasicBlockPrioComparator(const Module& module)
{
	size_t currPrio = 1;
	for (auto const& f: module)
	{
		if (f.isDeclaration())
			continue;
		for (auto itr = llvm::po_begin(&f), ite = llvm::po_end(&f); itr != ite; ++itr)
		{
			auto bb = *itr;
			for (auto itr = bb->rbegin(), ite = bb->rend(); itr != ite; ++itr)
			{
				prioMap[&*itr] = currPrio++;
			}
		}
	}
}

size_t BasicBlockPrioComparator::getPriority(const llvm::Instruction* bb) const
{
	auto itr = prioMap.find(bb);
	assert(itr != prioMap.end());
	return itr->second;
}

}
