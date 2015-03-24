#ifndef TPA_EXTERNAL_REF_TABLE_H
#define TPA_EXTERNAL_REF_TABLE_H

#include "PointerAnalysis/External/ExternalTable.h"

namespace tpa
{

enum class RefEffect
{
	UnknownEffect,
	NoEffect,
	RefArg0,
	RefArg1,
	RefArg0Arg1,
	RefAfterArg0,
	RefAfterArg1,
	RefArg1Array,
	RefAllArgs,
};

template <>
struct EffectTrait<RefEffect>
{
	static RefEffect DefaultEffect;
};

class ExternalRefTable: public ExternalTable<RefEffect>
{
private:
	void initializeTable();
public:
	ExternalRefTable()
	{
		initializeTable();
		finalizeTable();
	}
};

}

#endif
