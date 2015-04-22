#ifndef TPA_TAINT_DESCRIPTOR_H
#define TPA_TAINT_DESCRIPTOR_H

#include <cstdint>

namespace client
{
namespace taint
{

// TClass specify whether the taintness is on the value, on the memory location pointed to by the value, or on all memory locations that can be obtained from dereferencing pointers that are results of doing pointer arithmetics on the value
enum class TClass: uint8_t
{
	ValueOnly,
	DirectMemory,
	ReachableMemory
};

// TEnd specify whether the given record is a taint source, a taint sink, or a taint pipe (transfer taint from one end to another)
enum class TEnd: uint8_t
{
	Source,
	Sink,
	Pipe
};

// TPosition specify where the position of the taintness is
class TPosition
{
private:
	uint8_t argIdx;
	bool isRet, isAll;

	TPosition(uint8_t i, bool r, bool a): argIdx(i), isRet(r), isAll(a) {}
public:
	uint8_t getArgIndex() const { return argIdx; }
	bool isReturnPosition() const { return isRet; }
	bool isAllArgPosition() const { return isAll; }

	static TPosition getReturnPosition()
	{
		return TPosition(0, true, false);
	}
	static TPosition getArgPosition(uint8_t idx)
	{
		return TPosition(idx, false, false);
	}
	static TPosition getAfterArgPosition(uint8_t idx)
	{
		return TPosition(idx, false, true);
	}
};

}
}

#endif
