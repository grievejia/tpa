#include "Context/Context.h"
#include "Context/ProgramPoint.h"
#include "Util/IO/Context/Printer.h"

#include <llvm/Support/raw_ostream.h>

namespace util
{
namespace io
{

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const context::Context& ctx)
{
	if (ctx.isGlobalContext())
		os << "[GLOBAL]";
	else
		os << "[CTX(" << ctx.size() << ") " << &ctx << "]";
	return os;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const context::ProgramPoint& p)
{
	os << "(" << *p.getContext() << ", " << *p.getInstruction() << ")";
	return os;
}

}
}