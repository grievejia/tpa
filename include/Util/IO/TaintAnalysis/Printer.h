#pragma once

#include "TaintAnalysis/Lattice/TaintLattice.h"
#include "Util/IO/PointerAnalysis/Printer.h"

namespace taint
{
	class DefUseInstruction;
	class ProgramPoint;
}

namespace util
{
namespace io
{

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, taint::TaintLattice l);
llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const taint::DefUseInstruction&);
llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const taint::ProgramPoint&);

}
}
