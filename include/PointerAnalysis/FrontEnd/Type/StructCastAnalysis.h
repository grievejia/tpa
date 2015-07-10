#pragma once

#include "PointerAnalysis/FrontEnd/Type/CastMap.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class StructCastAnalysis
{
public:
	CastMap runOnModule(const llvm::Module&);
};

}
