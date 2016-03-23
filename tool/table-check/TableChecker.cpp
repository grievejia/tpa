#include "TableChecker.h"
#include "Annotation/ModRef/ExternalModRefTable.h"
#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Annotation/Taint/ExternalTaintTable.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace llvm;

static cl::opt<bool> OutputPlainText("plain", cl::desc("Output all missing function name without additional texts and fancy colors"));

static void printWarning(const StringRef& msg, const StringRef& funName)
{
	if (OutputPlainText)
	{
		outs() << funName << "\n";
	}
	else
	{
		outs().changeColor(raw_ostream::MAGENTA) << msg;
		outs().changeColor(raw_ostream::YELLOW) << ": " << funName << "\n";
		outs().resetColor();
	}
}

template <typename T>
TableChecker<T>::TableChecker(const StringRef& fileName)
{
	table = T::loadFromFile(fileName.data());
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
	TableChecker<ExternalTaintTable>(fileName).check(m);
}

// Explicit instantiation
template class TableChecker<ExternalPointerTable>;
template class TableChecker<ExternalModRefTable>;
template class TableChecker<ExternalTaintTable>;