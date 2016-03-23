#pragma once

#include "Dynamic/Log/LogReader.h"
#include "Dynamic/Log/LogVisitor.h"

namespace dynamic
{

template <typename SubClass, typename RetType = void>
class LogProcessor: public LogConstVisitor<SubClass, RetType>
{
private:
	LazyLogReader reader;
public:
	LogProcessor(const char* fileName): reader(fileName) {}

	void process()
	{
		while (auto rec = reader.readLogRecord())
			this->visit(*rec);
	}
};

}
