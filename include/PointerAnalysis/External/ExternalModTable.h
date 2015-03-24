#ifndef TPA_EXTERNAL_MOD_TABLE_H
#define TPA_EXTERNAL_MOD_TABLE_H

#include "PointerAnalysis/External/ExternalTable.h"

namespace tpa
{

enum class ModEffect
{
	UnknownEffect,
	NoEffect,
	ModArg0,
	ModArg1,
	ModAfterArg0,
	ModAfterArg1,
	ModArg0Array,
};

template <>
struct EffectTrait<ModEffect>
{
	static ModEffect DefaultEffect;
};

class ExternalModTable: public ExternalTable<ModEffect>
{
private:
	void initializeTable();
public:
	ExternalModTable()
	{
		initializeTable();
		finalizeTable();
	}
};

}

#endif
