#include "TableChecker.h"
#include "Client/Taintness/SourceSink/Table/SourceSinkLookupTable.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"
#include "PointerAnalysis/External/Pointer/ExternalPointerTable.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;
using namespace client::taint;

static void printWarning(const StringRef& msg, const StringRef& funName)
{
	errs().changeColor(raw_ostream::MAGENTA) << msg;
	errs().changeColor(raw_ostream::YELLOW) << ": " << funName << "\n";
	errs().resetColor();
}

template <typename T>
TableChecker<T>::TableChecker(const StringRef& fileName)
{
	table = T::loadFromFile(fileName);
}

template <typename T>
void TableChecker<T>::check(const Module& module)
{
	for (auto const& f: module)
	{
		auto fName = f.getName();
		auto summary = table.lookup(fName);
		if (f.isDeclaration())
		{
			if (summary == nullptr)
				printWarning("External function not annotated", fName);
		}
		else
		{
			if (summary != nullptr)
				printWarning("User-defined function annotated", fName);
		}
	}
}

void checkPtrEffectTable(const Module& m, const StringRef& fileName)
{
	TableChecker<ExternalPointerTable>(fileName).check(m);
}

void checkModRefEffectTable(const Module& m, const StringRef& fileName)
{
	TableChecker<ExternalModRefTable>(fileName).check(m);
}

void checkTaintTable(const Module& m , const StringRef& fileName)
{
	TableChecker<SourceSinkLookupTable>(fileName).check(m);
}

// Explicit instantiation
template class TableChecker<ExternalPointerTable>;
template class TableChecker<ExternalModRefTable>;
template class TableChecker<SourceSinkLookupTable>;