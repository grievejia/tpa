#include "TaintAnalysis/Program/DefUseInstruction.h"
#include "TaintAnalysis/Support/ProgramPoint.h"
#include "Util/IO/TaintAnalysis/Printer.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

using namespace taint;

namespace util
{
namespace io
{

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, TaintLattice l)
{
	switch (l)
	{
		case TaintLattice::Unknown:
			os << "Unknown";
			break;
		case TaintLattice::Untainted:
			os << "Untainted";
			break;
		case TaintLattice::Tainted:
			os << "Tainted";
			break;
		case TaintLattice::Either:
			os << "Either";
			break;
	}
	return os;
}

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const taint::DefUseInstruction& duInst)
{
	if (duInst.isEntryInstruction())
		os << "[entry duInst for " << duInst.getFunction()->getName() << "]";
	else
		os << "[" << duInst.getFunction()->getName() << *duInst.getInstruction() << "]";
	return os;
}

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const taint::ProgramPoint& pp)
{
	os << "(" << *pp.getContext() << ", " << *pp.getDefUseInstruction() << ")";
	return os;
}

}
}