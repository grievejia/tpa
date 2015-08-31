#include "PointerAnalysis/Program/CFG/CFGNode.h"
#include "PointerAnalysis/Support/FunctionContext.h"
#include "PointerAnalysis/Support/ProgramPoint.h"
#include "Util/IO/PointerAnalysis/Printer.h"
#include "Util/IO/PointerAnalysis/NodePrinter.h"

#include <llvm/IR/Function.h>

namespace util
{
namespace io
{

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::CFGNode& node)
{
	NodePrinter<tpa::CFGNode>(os).visit(node);
	return os;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::ProgramPoint& pp)
{
	os << "(" << *pp.getContext() << ", " << *pp.getCFGNode() << ")";
	return os;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::FunctionContext& fc)
{
	os << "(" << *fc.getContext() << ", " << fc.getFunction()->getName() << ")";
	return os;
}

}
}