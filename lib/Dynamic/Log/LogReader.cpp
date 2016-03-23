#include "Dynamic/Log/LogReader.h"

#include <cassert>
#include <iostream>

namespace dynamic
{

template <typename T>
static bool readData(std::istream& is, T* data)
{
	auto charPtr = reinterpret_cast<char*>(data);
	is.read(charPtr, sizeof(T));
	return is.good();
}

static std::experimental::optional<LogRecord> readRecord(std::istream& is)
{
	LogRecord rec;

	bool succ = true;
	char type;
	succ &= readData(is, &type);
	switch (type)
	{
		case TAllocRec:
			succ &= readData(is, &rec.allocRecord.type);
			succ &= readData(is, &rec.allocRecord.id);
			succ &= readData(is, &rec.allocRecord.address);
			break;
		case TPointerRec:
			succ &= readData(is, &rec.ptrRecord.id);
			succ &= readData(is, &rec.ptrRecord.address);
			break;
		case TEnterRec:
			succ &= readData(is, &rec.enterRecord.id);
			break;
		case TExitRec:
			succ &= readData(is, &rec.exitRecord.id);
			break;
		case TCallRec:
			succ &= readData(is, &rec.callRecord.id);
			break;
		default:
		{
			std::cerr << static_cast<unsigned>(rec.type) << std::endl;
			std::cerr << "Illegal record type. Log file must be broken.\n";
			std::exit(-1);
		}
	}
	rec.type = static_cast<LogRecordType>(type);

	if (!succ)
		return std::experimental::optional<LogRecord>();
	else
		return std::experimental::make_optional(std::move(rec));
}

std::vector<LogRecord> EagerLogReader::readLogFromFile(const char* fileName)
{
	std::vector<LogRecord> ret;

	std::ifstream file(fileName, std::ios::in|std::ios::binary|std::ios::ate);
	if (file.is_open())
	{
		auto size = file.tellg();
		auto numEntry = size / sizeof(LogRecord);
		ret.reserve(numEntry);

		file.seekg(0, std::ios::beg);
		for (auto i = 0; i < numEntry; ++i)
		{
			auto rec = readRecord(file);
			assert(rec && "Log read failure");
			ret.emplace_back(std::move(*rec));
		}
	}

	return ret;
}

LazyLogReader::LazyLogReader(const char* fileName): ifs(fileName, std::ios::in|std::ios::binary)
{
	if (!ifs.is_open())
	{
		std::cerr << "Open log file " << fileName << " failed\n";
		std::exit(-1);
	}
}

std::experimental::optional<LogRecord> LazyLogReader::readLogRecord()
{
	return readRecord(ifs);
}

}