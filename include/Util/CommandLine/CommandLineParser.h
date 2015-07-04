#pragma once

#include "Util/CommandLine/CommandLineFlags.h"

#include <llvm/ADT/StringRef.h>

#include <experimental/optional>

namespace util
{

class CommandLineParser
{
private:
	struct PositionalFlagEntry
	{
		llvm::StringRef desc;
	};

	struct OptionalFlagEntry
	{
		llvm::StringRef desc;
		std::experimental::optional<llvm::StringRef> defaultValue;
	};

	using OptionalFlagMap = VectorMap<llvm::StringRef, OptionalFlagEntry>;
	OptionalFlagMap optFlagMap;
	using PositionalFlagMap = std::vector<std::pair<llvm::StringRef, PositionalFlagEntry>>;
	PositionalFlagMap posFlagMap;

	llvm::StringRef helpMsg;

	void addOptionalFlagEntry(const llvm::StringRef& name, OptionalFlagEntry&& entry);
	void addPositionalFlagEntry(const llvm::StringRef& name, PositionalFlagEntry&& entry);
	void parseArgs(const std::vector<llvm::StringRef>& args, CommandLineFlags& result) const;
public:
	CommandLineParser(const llvm::StringRef& h = ""): helpMsg(h) {}

	void addPositionalFlag(const llvm::StringRef& name, const llvm::StringRef& desc);
	void addOptionalFlag(const llvm::StringRef& name, const llvm::StringRef& desc);
	void addOptionalFlag(const llvm::StringRef& name, const llvm::StringRef& desc, const llvm::StringRef& init);

	void printUsage(const llvm::StringRef& progName) const;

	CommandLineFlags parseCommandLineOptions(int, char**) const;
};

}
