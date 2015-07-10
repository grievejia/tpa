#pragma once

#include "PointerAnalysis/FrontEnd/Type/TypeSet.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class TypeCollector
{
public:
	TypeCollector() = default;

	TypeSet runOnModule(const llvm::Module&);
};

}
