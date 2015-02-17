#include "MemoryModel/Pointer/Pointer.h"
#include "MemoryModel/Precision/Context.h"
#include "MemoryModel/PtsSet/PtsEnv.h"
#include "MemoryModel/PtsSet/Store.h"
#include "MemoryModel/Memory/Memory.h"

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
		os << ctx->callSite->getName() << " ";
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

void Env::dump(raw_ostream& os) const
{
	os << "\n----- Env -----\n";
	for (auto const& mapping: env)
	{
		os << *mapping.first << "  -->>  { ";
		for (auto loc: *mapping.second)
			os << *loc << " ";
		os << "}\n";
	}
	os << "----- END -----\n\n";
}

void Store::dump(raw_ostream& os) const
{
	auto dumpBinding = [&os] (const MappingType& bindings)
	{
		for (auto const& mapping: bindings)
		{
			os << *mapping.first << "  -->>  { ";
			for (auto loc: *mapping.second)
				os << *loc << " ";
			os << "}\n";
		}
	};

	os << "\n----- Store -----\n";
	dumpBinding(globalMem);
	dumpBinding(stackMem);
	dumpBinding(heapMem);
	os << "----- END -----\n\n";
}

}
