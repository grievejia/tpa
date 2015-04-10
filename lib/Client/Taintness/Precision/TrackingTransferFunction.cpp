#include "Client/Taintness/Precision/TrackingTransferFunction.h"

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

TrackingTransferFunction::TrackingTransferFunction(const TaintGlobalState& g, ValueSet& v, MemorySet& m): globalState(g), valueSet(v), memSet(m)
{
}

void TrackingTransferFunction::eval(const tpa::DefUseInstruction*)
{
	
}

}
}