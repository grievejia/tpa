#ifndef TPA_PROGRAM_LOCATION_H
#define TPA_PROGRAM_LOCATION_H

#include "Precision/Context.h"

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

class ProgramLocation
{
private:
	const Context* context;
	const llvm::Value* inst;
public:
	explicit ProgramLocation(const Context* c, const llvm::Value* i): context(c), inst(i) {}

	const Context* getContext() const { return context; }
	const llvm::Value* getInstruction() const { return inst; }

	bool operator==(const ProgramLocation& other) const
	{
		return (context == other.context) && (inst == other.inst);
	}
	bool operator!=(const ProgramLocation& other) const
	{
		return !(*this == other);
	}
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const ProgramLocation& p);

}

namespace std
{
	template<> struct hash<tpa::ProgramLocation>
	{
		size_t operator()(const tpa::ProgramLocation& l) const
		{
			std::size_t seed = 0;
			tpa::hash_combine(seed, l.getContext());
			tpa::hash_combine(seed, l.getInstruction());
			return seed;
		}
	};
}

#endif
