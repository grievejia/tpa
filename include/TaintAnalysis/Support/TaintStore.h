#pragma once

#include "TaintAnalysis/Support/TaintMap.h"

namespace tpa
{
	class MemoryObject;
}

namespace taint
{

using TaintStore = TaintMap<const tpa::MemoryObject*>;

}