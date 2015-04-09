#ifndef TPA_TAINT_DESCRIPTOR_H
#define TPA_TAINT_DESCRIPTOR_H

namespace client
{
namespace taint
{

// TPosition specify where the position of the taintness is
enum class TPosition: std::uint8_t
{
	Ret,
	Arg0,
	Arg1,
	Arg2,
	Arg3,
	Arg4,
	AfterArg0,
	AfterArg1,
	AllArgs,
};

// TClass specify whether the taintness is on the value or on the memory location pointed to by the value
enum class TClass: bool
{
	ValueOnly,
	DirectMemory,
};

// TEnd specify whether the given record is a taint source or a taint sink
enum class TEnd: bool
{
	Source,
	Sink
};

}
}

#endif
