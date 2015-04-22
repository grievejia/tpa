#ifndef TPA_DEF_USE_PROGRAM_LOCATION_H
#define TPA_DEF_USE_PROGRAM_LOCATION_H

#include "MemoryModel/Precision/Context.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"

namespace tpa
{

class DefUseProgramLocation
{
private:
	const Context* context;
	const DefUseInstruction* duInst;

	using PairType = std::pair<const Context*, const DefUseInstruction*>;
public:
	explicit DefUseProgramLocation(const Context* c, const DefUseInstruction* i): context(c), duInst (i) {}
	DefUseProgramLocation(const PairType& p): context(p.first), duInst(p.second) {}

	const Context* getContext() const { return context; }
	const DefUseInstruction* getDefUseInstruction() const { return duInst; }

	bool operator==(const DefUseProgramLocation& other) const
	{
		return (context == other.context) && (duInst == other.duInst);
	}
	bool operator!=(const DefUseProgramLocation& other) const
	{
		return !(*this == other);
	}

	operator PairType() const
	{
		return std::make_pair(context, duInst);
	}
};

}

namespace std
{
	template<> struct hash<tpa::DefUseProgramLocation>
	{
		size_t operator()(const tpa::DefUseProgramLocation& l) const
		{
			std::size_t seed = 0;
			tpa::hash_combine(seed, l.getContext());
			tpa::hash_combine(seed, l.getDefUseInstruction());
			return seed;
		}
	};
}

#endif
