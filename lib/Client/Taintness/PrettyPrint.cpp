#include "Client/Lattice/TaintLattice.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace client
{
namespace taint
{

raw_ostream& operator<<(raw_ostream& os, TaintLattice t)
{
	switch (t)
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

}
}