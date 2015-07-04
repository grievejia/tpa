#include "Util/CommandLine/CommandLineParser.h"

#include <llvm/Support/raw_ostream.h>

#include <exception>

using namespace llvm;

namespace util
{

namespace
{

class ParseException: public std::exception
{
private:
	std::string msg;
public:
	ParseException(const std::string& m): msg(m) {}

	const char* what() const noexcept override
	{
		return msg.data();
	}
};

class HelpException: public std::exception
{
public:
	HelpException() = default;

	const char* what() const noexcept override
	{
		return "-help option is encountered";
	}
};

}

void CommandLineParser::printUsage(const StringRef& progName) const
{
	outs() << "Description: \n  " << helpMsg << "\n\n";

	outs() << "Usage:\n  " << progName << " [options] ";
	for (auto const& mapping: posFlagMap)
		outs() << "<" << mapping.first << "> ";
	outs() << "\n\n";

	outs() << "Positional arguments:\n";
	for (auto const& mapping: posFlagMap)
	{
		outs() << "  " << mapping.first << "\t\t\t" << mapping.second.desc << "\n";
	}
	outs() << "\n";

	outs() << "Optional arguments:\n";
	for (auto const& mapping: optFlagMap)
	{
		auto argName = "-" + mapping.first.str();
		if (mapping.second.defaultValue)
			argName += " <" + mapping.first.str() + "_arg>";
		outs() << "  " << argName << "\t\t\t" << mapping.second.desc;
		if (mapping.second.defaultValue && !(*mapping.second.defaultValue).empty())
			outs() << " (default = " << *mapping.second.defaultValue<< ")";
		outs() << "\n";
	}
}

void CommandLineParser::addOptionalFlagEntry(const StringRef& name, OptionalFlagEntry&& entry)
{
	auto inserted = optFlagMap.try_emplace(name, std::move(entry)).second;
	assert(inserted && "Duplicate entry in optFlagMap!");
}

void CommandLineParser::addPositionalFlagEntry(const StringRef& name, PositionalFlagEntry&& entry)
{
	assert(std::find_if(
		posFlagMap.begin(),
		posFlagMap.end(),
		[&name] (auto const& pair)
		{
			return name == pair.first;
		}
	) == posFlagMap.end() && "Duplicate entry in posFlagMap!");
	
	posFlagMap.emplace_back(std::make_pair(name, std::move(entry)));
}

void CommandLineParser::addPositionalFlag(const StringRef& name, const StringRef& desc)
{
	PositionalFlagEntry entry = { desc };
	addPositionalFlagEntry(name, std::move(entry));
}

void CommandLineParser::addOptionalFlag(const StringRef& name, const StringRef& desc)
{
	OptionalFlagEntry entry = { desc, std::experimental::nullopt };
	addOptionalFlagEntry(name, std::move(entry));
}

void CommandLineParser::addOptionalFlag(const StringRef& name, const StringRef& desc, const StringRef& init)
{
	OptionalFlagEntry entry = { desc, init };
	addOptionalFlagEntry(name, std::move(entry));
}

static bool isOption(const StringRef& str)
{
	assert(!str.empty());
	return str.front() == '-';
}

void CommandLineParser::parseArgs(const std::vector<StringRef>& args, CommandLineFlags& result) const
{
	size_t numPositionalConsumed = 0;
	size_t numFlagConsumed = 0;

	while (numFlagConsumed < args.size())
	{
		auto flag = args[numFlagConsumed];
		if (!isOption(flag))	// Positional arguments
		{
			if (numPositionalConsumed >= posFlagMap.size())
				throw ParseException("too many positional arguments");

			auto inserted = result.addFlag(posFlagMap[numPositionalConsumed].first, flag);
			assert(inserted && "Insertion failed");
			++numPositionalConsumed;
		}
		else	// Optional arguments
		{
			flag = flag.drop_front();

			// See if flag is "-help"
			if (flag == "help")
				throw HelpException();

			auto itr = optFlagMap.find(flag);
			if (itr == optFlagMap.end())
				throw ParseException("Option not recognized: " + flag.str());

			if (itr->second.defaultValue)
			{
				++numFlagConsumed;
				if (numFlagConsumed >= args.size())
					throw ParseException("More argument needs to be provided after option " + flag.str());
				auto flagArg = args[numFlagConsumed];
				if (isOption(flagArg))
					throw ParseException("Found another option instead of an argument after option " + flag.str());

				auto inserted = result.addFlag(flag, flagArg);
				assert(inserted && "Insertion failed");
			}
			else
			{
				auto inserted = result.addFlag(flag, "");
				assert(inserted && "Insertion failed");
			}
		}
		++numFlagConsumed;
	}

	if (numPositionalConsumed < posFlagMap.size())
		throw ParseException("Not enough positional arguments are provided");

	for (auto const& mapping: optFlagMap)
	{
		if (mapping.second.defaultValue && result.lookup(mapping.first) == nullptr)
			result.addFlag(mapping.first, *mapping.second.defaultValue);
	}
}

CommandLineFlags CommandLineParser::parseCommandLineOptions(int argc, char** argv) const
{
	assert(argc > 0 && "argc cannot be nonpositive!");

	CommandLineFlags result;

	try
	{
		std::vector<StringRef> args;
		args.reserve(argc);
		for (auto i = 1; i < argc; ++i)
			args.emplace_back(argv[i]);

		parseArgs(args, result);
	}
	catch (const ParseException& e)
	{
		errs() << "Command line options parsing failed: " << e.what() << "\n\n";
		errs() << "Please run \"" << argv[0] << " -help\" to view usage information\n";
		std::exit(-1);
	}
	catch (const HelpException& e)
	{
		printUsage(argv[0]);
		std::exit(0);
	}

	return result;
}

}
