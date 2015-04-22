#ifndef PCOMB_PREDICATE_CHAR_PARSER_H
#define PCOMB_PREDICATE_CHAR_PARSER_H

#include "Utils/pcomb/Parser/Parser.h"

namespace pcomb
{

// PredicateCharParser matches a char that satisfies a predicate and returns that char as its attribute
template <typename Pred>
class PredicateCharParser: public Parser<char>
{
private:
	Pred pred;
public:
	using OutputType = char;
	using ResultType = typename Parser<char>::ResultType;

	PredicateCharParser(const Pred& p): pred(p) {}
	PredicateCharParser(Pred&& p): pred(std::move(p)) {}

	ResultType parse(const InputStream& input) const override
	{
		auto ret = ResultType(input);

		if (!input.isEOF())
		{
			auto firstChar = input.getRawBuffer()[0];
			if (pred(firstChar))
				ret = ResultType(input.consume(1), firstChar);
		}
		
		return ret;
	}
};

namespace detail
{

class CharEqPredicate
{
private:
	char ch;
public:
	CharEqPredicate(char c): ch(c) {}

	bool operator()(char c) const
	{
		return c == ch;
	}
};

class CharRangePredicate
{
private:
	char lo, hi;
public:
	CharRangePredicate(char l, char h): lo(l), hi(h) {}

	bool operator()(char c) const
	{
		return c >= lo && c <= hi;
	}
};

}	// end of namespace detail

inline PredicateCharParser<detail::CharEqPredicate> ch(char c)
{
	return PredicateCharParser<detail::CharEqPredicate>(detail::CharEqPredicate(c));
}

// The enable_if here is to avoid instantiating this template when CustomPredicate is char
template <typename CustomPredicate>
std::enable_if_t<!std::is_same<CustomPredicate, char>::value, PredicateCharParser<CustomPredicate>> ch(CustomPredicate p)
{
	return PredicateCharParser<CustomPredicate>(std::move(p));
}

inline PredicateCharParser<detail::CharRangePredicate> range(char l, char h)
{
	return PredicateCharParser<detail::CharRangePredicate>(detail::CharRangePredicate(l, h));
}

}

#endif
