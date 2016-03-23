#include "Dynamic/Instrument/AllocType.h"
#include "Dynamic/Log/LogPrinter.h"

#include <iostream>

namespace dynamic
{

LogPrinter::LogPrinter(const char* fileName, std::ostream& o): LogProcessor(fileName), os(o) {}

void LogPrinter::visitAllocRecord(const AllocRecord& allocRecord)
{
	os << "[ALLOC] ";
	switch (allocRecord.type)
	{
		case AllocType::Global:
			os << "Global ";
			break;
		case AllocType::Stack:
			os << "Stack ";
			break;
		case AllocType::Heap:
			os << "Heap ";
			break;
		default:
			std::cerr << "Illegal alloc type. Log file must be broken\n";
			std::exit(-1);
	}
	os << "Ptr# " << allocRecord.id << " = " << allocRecord.address << '\n';
}

void LogPrinter::visitPointerRecord(const PointerRecord& ptrRecord)
{
	os << "[POINTER] Ptr# " << ptrRecord.id << " = " << ptrRecord.address << '\n';
}

void LogPrinter::visitEnterRecord(const EnterRecord& enterRecord)
{
	os << "[ENTER] Function# " << enterRecord.id << '\n';
}

void LogPrinter::visitExitRecord(const ExitRecord& exitRecord)
{
	os << "[EXIT] Function# " << exitRecord.id << '\n';
}

void LogPrinter::visitCallRecord(const CallRecord& callRecord)
{
	os << "[CALL] Inst# " << callRecord.id << '\n';
}

}
