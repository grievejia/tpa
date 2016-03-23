#pragma once

#include "Util/CommandLine/CommandLineParser.h"

namespace util
{

class TypedCommandLineParser
{
private:
	enum class OptionType
	{
		String,
		Bool,
		UInt,
	};

	struct OptionEntry
	{
		OptionType type;
		void* data;
	};

	VectorMap<llvm::StringRef, OptionEntry> optionMap;

	CommandLineParser parser;

	void updateOptionMap(const llvm::StringRef&, OptionType, void*);
public:
	TypedCommandLineParser(const llvm::StringRef& h = ""): parser(h) {}

	void addBooleanOptionalFlag(const llvm::StringRef&, const llvm::StringRef&, bool&);
	void addStringOptionalFlag(const llvm::StringRef&, const llvm::StringRef&, llvm::StringRef&);
	void addUIntOptionalFlag(const llvm::StringRef&, const llvm::StringRef&, unsigned&);
	void addStringPositionalFlag(const llvm::StringRef&, const llvm::StringRef&, llvm::StringRef&);
	void parseCommandLineOptions(int argc, char** argv);
};

}
