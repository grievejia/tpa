#ifndef TPA_UTIL_TABLE_PRINTER_H
#define TPA_UTIL_TABLE_PRINTER_H

#include "Client/Taintness/SourceSink/Table/TablePrinter.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTablePrinter.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTablePrinter.h"

#include <llvm/Support/raw_ostream.h>

namespace table
{

template <typename T>
struct TablePrinter
{
	// Members to include:
	// static void printTable(llvm::raw_ostream&, const T& table);

	using UnknownTableType = typename T::UnknownTableTypeError;
};

template <>
struct TablePrinter<tpa::ExternalPointerTable>
{
	static void printTable(llvm::raw_ostream& os, const tpa::ExternalPointerTable& table)
	{
		tpa::ExternalPointerTablePrinter(os).printTable(table);
	}
};

template <>
struct TablePrinter<client::taint::ExternalTaintTable>
{
	static void printTable(llvm::raw_ostream& os, const client::taint::ExternalTaintTable& table)
	{
		client::taint::TablePrinter(os).printTable(table);
	}
};

template <>
struct TablePrinter<tpa::ExternalModRefTable>
{
	static void printTable(llvm::raw_ostream& os, const tpa::ExternalModRefTable& table)
	{
		tpa::ExternalModRefTablePrinter(os).printTable(table);
	}
};

}

#endif
