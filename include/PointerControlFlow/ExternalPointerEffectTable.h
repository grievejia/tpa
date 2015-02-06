#ifndef TPA_EXTERNAL_POINTER_EFFECT_H
#define TPA_EXTERNAL_POINTER_EFFECT_H

#include <llvm/ADT/StringRef.h>

#include <vector>

namespace tpa
{

enum class PointerEffect
{
	UnknownEffect,
	NoEffect,
	Malloc,
	Realloc,
	ReturnArg0,
	ReturnArg1,
	ReturnArg2,
	ReturnStatic,
	StoreArg0ToArg1,
	MemcpyArg0ToArg1,
	Memset,
};

class ExternalPointerEffectTable
{
private:
	using MappingType = std::pair<llvm::StringRef, PointerEffect>;
	// Since this table is used in a strict query-after-insertion scenerio, we just need a sorted vector to hold all mappings
	std::vector<MappingType> table;

	void setUpInitialTable();
public:
	explicit ExternalPointerEffectTable() { setUpInitialTable(); }

	PointerEffect lookup(llvm::StringRef name) const;
};

}

#endif
