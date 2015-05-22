#ifndef TPA_UTIL_TABLE_VALIDATOR_H
#define TPA_UTIL_TABLE_VALIDATOR_H

#include "Client/Taintness/SourceSink/Table/TablePrinter.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTablePrinter.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTablePrinter.h"

#include <llvm/Support/raw_ostream.h>

namespace table
{

template <typename T>
struct TableValidator
{
	// Members to include:
	// static void validateTable(llvm::raw_ostream&, const T& table);

	using UnknownTableType = typename T::UnknownTableTypeError;
};

template <>
struct TableValidator<tpa::ExternalPointerTable>
{
	static void validateTable(const tpa::ExternalPointerTable& table);
};

template <>
struct TableValidator<client::taint::ExternalTaintTable>
{
	static void validateTable(const client::taint::ExternalTaintTable& table);
};

template <>
struct TableValidator<tpa::ExternalModRefTable>
{
	static void validateTable(const tpa::ExternalModRefTable& table);
};

}

#endif