#include "Dynamic/Instrument/IDAssigner.h"

#include <llvm/IR/User.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace dynamic
{

static constexpr unsigned startID = 1u;

bool IDAssigner::assignValueID(const Value* v)
{
	assert(v != nullptr);

	if (idMap.count(v))
		return false;

	idMap[v] = nextID;
	assert(nextID == startID + revIdMap.size());
	revIdMap.push_back(v);
	++nextID;
	return true;
}

bool IDAssigner::assignUserID(const User* u)
{
	assert(u != nullptr);

	if (!assignValueID(u))
		return false;

	bool changed = false;
	for (auto const& op: u->operands())
		if (auto child = dyn_cast<User>(&op))
			changed |= assignUserID(child);
	return changed;
}

IDAssigner::IDAssigner(const Module& module): nextID(startID)
{
	for (auto const& g: module.globals())
	{
		assignValueID(&g);
		//if (g.hasInitializer())
		//	assignUserID(g.getInitializer());
	}

	for (auto const& f: module)
	{
		assignValueID(&f);
		
		for (auto const& arg: f.args())
			assignValueID(&arg);

		for (auto const& bb: f)
		{
			//assignValueID(&bb);
			for (auto const& inst: bb)
				assignValueID(&inst);
		}
	}
}

const IDAssigner::IDType* IDAssigner::getID(const llvm::Value* v) const
{
	assert(v != nullptr && "getID() received a NULL pointer!");
	auto itr = idMap.find(v);
	if (itr == idMap.end())
		return nullptr;
	else
		return &itr->second;
}

const llvm::Value* IDAssigner::getValue(IDType id) const
{
	if (id >= revIdMap.size())
		return nullptr;
	else
		return revIdMap[id - startID];
}

void IDAssigner::dump() const
{
	for (auto i = 0ul, e = revIdMap.size(); i < e; ++i)
	{
		errs() << (i + startID) << " => " << revIdMap[i]->getName() << "\n";
	}
}

}
