#ifndef TPA_PRECISION_LOSS_POSITION_SET_H
#define TPA_PRECISION_LOSS_POSITION_SET_H

#include <bitset>

namespace client
{
namespace taint
{

template <size_t MaxNumArgument = 7>
class PrecisionLossPositionSet
{
private:
	// Index 0 is reserved for function return position
	std::bitset<MaxNumArgument + 1> posSet;
public:
	PrecisionLossPositionSet() = default;

	void setReturnLossPosition()
	{
		posSet.set(0);
	}

	void setArgumentLossPosition(size_t idx)
	{
		posSet.set(idx + 1);
	}

	bool isReturnLossPosition() const
	{
		return posSet[0];
	}

	bool isArgumentLossPosition(size_t idx) const
	{
		return posSet[idx + 1];
	}

	size_t size() const
	{
		return posSet.size() - 1;
	}

	size_t count() const
	{
		return posSet.count();
	}

	size_t empty() const
	{
		return posSet.none();
	}
};

}
}

#endif
