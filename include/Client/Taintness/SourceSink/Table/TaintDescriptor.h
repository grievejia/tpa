#ifndef TPA_TAINT_DESCRIPTOR_H
#define TPA_TAINT_DESCRIPTOR_H

#include "PointerAnalysis/External/Descriptors.h"

namespace client
{
namespace taint
{

// TClass specify whether the taintness is on the value, on the memory location pointed to by the value, or on all memory locations that can be obtained from dereferencing pointers that are results of doing pointer arithmetics on the value
using TClass = tpa::AClass;

// TEnd specify whether the given record is a taint source, a taint sink, or a taint pipe (transfer taint from one end to another)
enum class TEnd: uint8_t
{
	Source,
	Sink,
	Pipe
};

// TPosition specify where the position of the taintness is
using TPosition = tpa::APosition;


}
}

#endif
