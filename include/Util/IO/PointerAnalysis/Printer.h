#pragma once

#include "Util/IO/Context/Printer.h"

namespace llvm
{
	class Value;
}

namespace tpa
{
	class AllocSite;
	class Pointer;
	class MemoryObject;
	class PtsSet;

	class ArrayLayout;
	struct ArrayTriple;
	class PointerLayout;
	class TypeLayout;

	class CFGNode;
	class ProgramPoint;
	class FunctionContext;
}

namespace util
{
namespace io
{

void dumpValue(llvm::raw_ostream& os, const llvm::Value& value);

llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const tpa::AllocSite&);
llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const tpa::Pointer&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::MemoryObject&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::PtsSet&);

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::ArrayTriple&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::ArrayLayout&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::PointerLayout&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::TypeLayout&);

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::CFGNode&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::ProgramPoint&);
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const tpa::FunctionContext&);

}
}

using namespace util::io;
