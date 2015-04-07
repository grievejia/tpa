#ifndef TPA_EVAL_RESULT_H
#define TPA_EVAL_RESULT_H

#include <experimental/optional>
#include <tuple>

namespace tpa
{

class EvalStatus
{
private:
	using ResultPair = std::pair<bool, bool>;

	std::experimental::optional<ResultPair> result;

	EvalStatus() = default;
	EvalStatus(bool e, bool s): result(std::make_pair(e, s)) {}
public:

	bool isValid() const { return static_cast<bool>(result); }
	bool hasEnvChanged() const
	{
		assert(isValid());
		return (*result).first;
	}
	bool hasStoreChanged() const
	{
		assert(isValid());
		return (*result).second;
	}

	static EvalStatus getInvalidStatus()
	{
		return EvalStatus();
	}

	static EvalStatus getValidStatus(bool e, bool s)
	{
		return EvalStatus(e, s);
	}
};

}

#endif
