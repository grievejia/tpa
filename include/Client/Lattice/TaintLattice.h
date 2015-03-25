#ifndef TPA_INFOFLOW_LATTICE_H
#define TPA_INFOFLOW_LATTICE_H

#include "Client/Lattice/Lattice.h"

namespace client
{

enum class TaintLattice
{
	Untainted,
	Tainted,
	Either
};

template<> struct Lattice<TaintLattice>
{
	static LatticeCompareResult compare(const TaintLattice& lhs, const TaintLattice& rhs)
	{
		if (lhs == rhs)
			return LatticeCompareResult::Equal;
		else if (rhs == TaintLattice::Either)
			return LatticeCompareResult::LessThan;
		else if (lhs == TaintLattice::Either)
			return LatticeCompareResult::GreaterThan;
		else
			return LatticeCompareResult::Incomparable;
	}
	static TaintLattice merge(const TaintLattice& lhs, const TaintLattice& rhs)
	{
		if (lhs == rhs)
			return lhs;
		else
			return TaintLattice::Either;
	}
};

}

#endif
