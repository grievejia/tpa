#include "Util/CommandLine/TypedCommandLineParser.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace util
{

void TypedCommandLineParser::updateOptionMap(const StringRef& name, const OptionType type, void* data)
{
	auto inserted = optionMap.try_emplace(name, OptionEntry{type, data}).second;
	assert(inserted && "Duplicate entry in optionMap!");
}

void TypedCommandLineParser::addUIntOptionalFlag(const StringRef& name, const StringRef& desc, unsigned& flag)
{
	parser.addOptionalFlag(name, desc, "");
	updateOptionMap(name, OptionType::UInt, &flag);
}

void TypedCommandLineParser::addBooleanOptionalFlag(const StringRef& name, const StringRef& desc, bool& flag)
{
	parser.addOptionalFlag(name, desc);
	updateOptionMap(name, OptionType::Bool, &flag);
}

void TypedCommandLineParser::addStringOptionalFlag(const StringRef& name, const StringRef& desc, StringRef& str)
{
	parser.addOptionalFlag(name, desc, "");
	updateOptionMap(name, OptionType::String, &str);
}

void TypedCommandLineParser::addStringPositionalFlag(const StringRef& name, const StringRef& desc, StringRef& str)
{
	parser.addPositionalFlag(name, desc);
	updateOptionMap(name, OptionType::String, &str);
}

void TypedCommandLineParser::parseCommandLineOptions(int argc, char** argv)
{
	auto parseResult = parser.parseCommandLineOptions(argc, argv);

	for (auto& mapping: optionMap)
	{
		auto data = mapping.second.data;
		switch (mapping.second.type)
		{
			case OptionType::Bool:
			{
				*(reinterpret_cast<bool*>(data)) = parseResult.count(mapping.first);
				break;
			}
			case OptionType::UInt:
			{
				auto res = parseResult.lookup(mapping.first);
				assert(res != nullptr);
				if (!res->empty())
					*(reinterpret_cast<unsigned*>(data)) = std::stoul(res->str());
				break;
			}
			case OptionType::String:
			{
				auto res = parseResult.lookup(mapping.first);
				assert(res != nullptr);
				if (!res->empty())
					*(reinterpret_cast<StringRef*>(data)) = *res;
				break;
			}
		}
	}
}

}
