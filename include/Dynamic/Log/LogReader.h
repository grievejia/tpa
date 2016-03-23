#pragma once

#include "Dynamic/Log/LogRecord.h"

#include <experimental/optional>
#include <fstream>
#include <vector>

namespace dynamic
{

class EagerLogReader
{
public:
	EagerLogReader() = delete;

	static std::vector<LogRecord> readLogFromFile(const char* fileName);
};

class LazyLogReader
{
private:
	std::ifstream ifs;
public:
	LazyLogReader(const char* fileName);

	std::experimental::optional<LogRecord> readLogRecord();
};

}
