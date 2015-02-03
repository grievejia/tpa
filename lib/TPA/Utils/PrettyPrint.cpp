#include "Precision/Context.h"
#include "Memory/Memory.h"
#include "Memory/Pointer.h"

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

}
