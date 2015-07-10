#pragma once

#include "PointerAnalysis/FrontEnd/Type/TypeMap.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class TypeAnalysis
{
public:
	TypeAnalysis() = default;

	TypeMap runOnModule(const llvm::Module&);
};

}