#ifndef TPA_POINTER_H
#define TPA_POINTER_H

#include "MemoryModel/Precision/Context.h"

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

// The Pointer class models all SSA-variables in LLVM IR that has pointer type
class Pointer
{
private:
	const Context* ctx;
	const llvm::Value* value;

	Pointer(const Context* c, const llvm::Value* v): ctx(c), value(v) {}
public:
	const llvm::Value* getValue() const { return value; }
	const Context* getContext() const { return ctx; }

	bool operator==(const Pointer& other) const
	{
		return (ctx == other.ctx) && (value == other.value);
	}
	bool operator!=(const Pointer& other) const
	{
		return !(*this == other);
	}

	// For dumping variables onto screen. Defined in PrettyPrint.cpp
	friend llvm::raw_ostream& operator<< (llvm::raw_ostream& stream, const Pointer& v);
	friend class PointerManager;
};

}

namespace std
{
	template<> struct hash<tpa::Pointer>
	{
		size_t operator()(const tpa::Pointer& p) const
		{
			std::size_t seed = 0;
			tpa::hash_combine(seed, p.getValue());
			tpa::hash_combine(seed, p.getContext());
			return seed;
		}
	};
}

#endif
