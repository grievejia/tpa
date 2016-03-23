#pragma once

#include "Annotation/Pointer/ExternalPointerTable.h"

namespace llvm
{
	class Module;
}

namespace dynamic
{

class MemoryInstrument
{
private:
	annotation::ExternalPointerTable extTable;
public:
	MemoryInstrument() = default;

	void loadExternalTable(const char* fileName)
	{
		extTable = annotation::ExternalPointerTable::loadFromFile(fileName);
	}

	void runOnModule(llvm::Module&);
};

}
