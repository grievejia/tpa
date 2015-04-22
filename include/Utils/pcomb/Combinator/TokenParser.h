#ifndef PCOMB_TOKEN_PARSER_H
#define PCOMB_TOKEN_PARSER_H

#include "Utils/pcomb/Parser/Parser.h"

namespace pcomb
{

// TokenParser takes a parser p and strip the whitespace of the input string before feed it into p
template <typename ParserA>
class TokenParser: public Parser<typename ParserA::OutputType>
{
private:
	ParserA pa;
public:
	static_assert(std::is_base_of<Parser<typename ParserA::OutputType>, ParserA>::value, "TokenParser only accepts parser type");

	using OutputType = typename Parser<typename ParserA::OutputType>::OutputType;
	using ResultType = typename Parser<typename ParserA::OutputType>::ResultType;

	TokenParser(const ParserA& p): pa(p) {}
	TokenParser(ParserA&& p): pa(std::move(p)) {}

	ResultType parse(const InputStream& input) const override
	{
		auto resStream = input;
		while (!resStream.isEOF())
		{
			auto firstChar = resStream.getRawBuffer()[0];
			switch (firstChar)
			{
				case ' ':
				case '\t':
				case '\n':
				case '\v':
				case '\f':
				case '\r':
					resStream = resStream.consume(1);
					continue;
				default:
					break;
			}
			break;
		}
		return pa.parse(resStream);
	}
};

template <typename ParserA>
auto token(ParserA&& p)
{
	using ParserType = std::remove_reference_t<ParserA>;
	return TokenParser<ParserType>(std::forward<ParserA>(p));
}

}

#endif
