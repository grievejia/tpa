#pragma once

#include "PointerAnalysis/Support/PtsMap.h"

namespace tpa
{

class Pointer;
using Env = PtsMap<const Pointer*>;

}