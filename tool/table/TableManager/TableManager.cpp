#include "table.h"
#include "TableManager/TableManager.h"
#include "TableManager/Printer/TablePrinter.h"
#include "TableManager/Validator/TableValidator.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;
using namespace client::taint;

namespace table
{

template <typename Table>
TableManager<Table>::TableManager(const StringRef& fileName)
{
	table = Table::loadFromFile(fileName);
}

template <typename Table>
void TableManager<Table>::processCommand(CommandType cmdType)
{
	switch (cmdType)
	{
		case CommandType::List:
		{
			TablePrinter<Table>::printTable(outs(), table);
			break;
		}
		case CommandType::Validate:
		{
			TableValidator<Table>::validateTable(table);
			break;
		}
	}
}

// Explicit instantiation
template class TableManager<ExternalPointerTable>;
template class TableManager<ExternalModRefTable>;
template class TableManager<ExternalTaintTable>;

}