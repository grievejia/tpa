#ifndef TPA_EXTERNAL_POINTER_EFFECT_H
#define TPA_EXTERNAL_POINTER_EFFECT_H

#include "PointerAnalysis/External/ExternalTable.h"

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
	MemcpyArg1ToArg0,
	Memset,
};

template <>
struct EffectTrait<PointerEffect>
{
	static PointerEffect DefaultEffect;
};

class ExternalPointerEffectTable: public ExternalTable<PointerEffect>
{
private:
	void initializeTable();
public:
	ExternalPointerEffectTable()
	{
		initializeTable();
		finalizeTable();
	}
};

}

#endif
