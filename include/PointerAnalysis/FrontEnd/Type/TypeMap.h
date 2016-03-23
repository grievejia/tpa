#pragma once

#include "PointerAnalysis/FrontEnd/ConstPointerMap.h"

namespace llvm
{
	class Type;
}

namespace tpa
{

class TypeLayout;

using TypeMap = ConstPointerMap<llvm::Type, TypeLayout>;

}