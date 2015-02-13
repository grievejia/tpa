#ifndef TPA_INFOFLOW_LATTICE_H
#define TPA_INFOFLOW_LATTICE_H

#include "Client/Lattice/Lattice.h"

namespace tpa
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
	static LatticeType merge(const LatticeType& lhs, const LatticeType& rhs)
	{
		if (lhs == TaintLattice::Tainted || rhs == TaintLattice::Tainted)
			return TaintLattice::Tainted;
		else
			return TaintLattice::Untainted;
	}
};

}

#endif
