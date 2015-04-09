#ifndef TPA_TAINT_EVAL_RESULT_H
#define TPA_TAINT_EVAL_RESULT_H

#include <experimental/optional>
#include <tuple>

namespace client
{
namespace taint
{

class TaintEvalStatus
{
private:
	using ResultPair = std::pair<bool, bool>;

	std::experimental::optional<ResultPair> result;

	TaintEvalStatus() = default;
	TaintEvalStatus(bool e, bool s): result(std::make_pair(e, s)) {}
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

	TaintEvalStatus operator||(const TaintEvalStatus& rhs) const
	{
		if (!isValid() || !rhs.isValid())
			return TaintEvalStatus();
		else
			return TaintEvalStatus(hasEnvChanged() || rhs.hasEnvChanged(), hasStoreChanged() || rhs.hasStoreChanged());
	}

	static TaintEvalStatus getInvalidStatus()
	{
		return TaintEvalStatus();
	}

	static TaintEvalStatus getValidStatus(bool e, bool s)
	{
		return TaintEvalStatus(e, s);
	}
};

}
}

#endif
