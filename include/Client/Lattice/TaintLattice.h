#ifndef TPA_INFOFLOW_LATTICE_H
#define TPA_INFOFLOW_LATTICE_H

#include "Client/Lattice/Lattice.h"

namespace client
{

enum class TaintLattice: uint8_t
{
	Unknown,
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
		else if (rhs == TaintLattice::Either || lhs == TaintLattice::Unknown)
			return LatticeCompareResult::LessThan;
		else if (lhs == TaintLattice::Either || rhs == TaintLattice::Unknown)
			return LatticeCompareResult::GreaterThan;
		else
			return LatticeCompareResult::Incomparable;
	}
	static TaintLattice merge(const TaintLattice& lhs, const TaintLattice& rhs)
	{
		auto cmpResult = compare(lhs, rhs);
		switch (cmpResult)
		{
			case LatticeCompareResult::Equal:
			case LatticeCompareResult::GreaterThan:
				return rhs;
			case LatticeCompareResult::LessThan:
				return lhs;
			case LatticeCompareResult::Incomparable:
				return TaintLattice::Either;
		}
	}
};

}

#endif
