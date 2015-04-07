#ifndef TPA_SEMI_SPARSE_GLOBAL_STATE_H
#define TPA_SEMI_SPARSE_GLOBAL_STATE_H

#include "PointerAnalysis/ControlFlow/PointerProgram.h"
#include "TPA/DataFlow/GlobalState.h"

namespace tpa
{

using SemiSparseGlobalState = GlobalState<PointerProgram>;

}

#endif
