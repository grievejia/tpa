#include "MemoryModel/Pointer/Pointer.h"
#include "MemoryModel/PtsSet/Env.h"
#include "MemoryModel/PtsSet/Store.h"

#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

raw_ostream& operator<< (raw_ostream& os, const Context& c)
{
	os << "< ";
	auto ctx = &c;
	while (ctx->size > 0)
	{
		ImmutableCallSite cs(ctx->callSite);
		assert(cs);
		os << cs.getCalledValue()->getName() << " ";
		ctx = ctx->predContext;
	}
	os << ">";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const Pointer& p)
{
	auto value = p.getValue();
	if (value != nullptr)
		os << "Ptr[" << *p.getContext() << ", " << value->getName() << "]";
	else
		os << "Ptr[sp]";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const ProgramLocation& p)
{
	assert(p.getContext() != nullptr);
	auto inst = p.getInstruction();
	if (inst != nullptr)
		os << *p.getContext() << "::" << inst->getName() << "";
	else
		os << *p.getContext() << "::MysteriousLoc";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const MemoryObject& o)
{
	os << "MemObj(" << o.getAllocationSite() << ", " << o.getSize() << ")";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const MemoryLocation& l)
{
	os << "MemLoc";
	if (l.isSummaryLocation())
		os << "[]";
	os << "(" << l.getMemoryObject()->getAllocationSite() << ", " << l.getOffset() << ")";
	return os;
}

template <typename Map>
void dumpMap(raw_ostream& os, const Map& map, StringRef title)
{
	os << "\n----- " << title << " -----\n";
	for (auto const& mapping: map)
	{
		os << *mapping.first << "  -->>  { ";
		for (auto loc: mapping.second)
			os << *loc << " ";
		os << "}\n";
	}
	os << "----- END -----\n\n";
}

raw_ostream& operator<<(raw_ostream& os, const Env& env)
{
	dumpMap(os, env, "Env");
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const Store& store)
{
	dumpMap(os, store, "Store");
	return os;
}

}
