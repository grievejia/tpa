#ifndef TPA_UTIL_TABLE_MANAGER_H
#define TPA_UTIL_TABLE_MANAGER_H

#include "table.h"

#include "Client/Taintness/SourceSink/Table/ExternalTaintTable.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"

#include <llvm/ADT/StringRef.h>

namespace table
{

template <typename Table>
class TableManager
{
private:
	Table table;
public:
	TableManager(const llvm::StringRef& fileName);

	void processCommand(CommandType cmdType);
};

using PointerTableManager = TableManager<tpa::ExternalPointerTable>;
using TaintTableManager = TableManager<client::taint::ExternalTaintTable>;
using ModRefTableManager = TableManager<tpa::ExternalModRefTable>;

}

#endif
