#ifndef TPA_INFOFLOW_LATTICE_H
#define TPA_INFOFLOW_LATTICE_H

#include "Client/Lattice/Lattice.h"

namespace client
{

enum class TaintLattice
{
	Untainted,
	Tainted,
};

template<> struct Lattice<TaintLattice>
{
	static LatticeCompareResult compare(const TaintLattice& lhs, const TaintLattice& rhs)
	{
		if (lhs == rhs)
			return LatticeCompareResult::Equal;
		else if (lhs == TaintLattice::Untainted)
			return LatticeCompareResult::LessThan;
		else
			return LatticeCompareResult::GreaterThan;
	}
	static TaintLattice merge(const TaintLattice& lhs, const TaintLattice& rhs)
	{
		if (lhs == TaintLattice::Tainted || rhs == TaintLattice::Tainted)
			return TaintLattice::Tainted;
		else
			return TaintLattice::Untainted;
	}
};

}

#endif
