#include "CommandLineOptions.h"
#include "RunAnalysis.h"

#include "PointerAnalysis/Analysis/GlobalPointerAnalysis.h"
#include "PointerAnalysis/FrontEnd/Type/TypeAnalysis.h"
#include "PointerAnalysis/MemoryModel/MemoryManager.h"
#include "PointerAnalysis/MemoryModel/PointerManager.h"
#include "PointerAnalysis/Support/Env.h"
#include "PointerAnalysis/Support/Store.h"
#include "Util/IO/PointerAnalysis/Printer.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>

using namespace llvm;
using namespace tpa;
using namespace util::io;

namespace
{

void dumpEnv(const Env& env)
{
	outs() << "Env:\n";
	for (auto const& mapping: env)
	{
		outs() << "  " << *mapping.first << "  ->  " << mapping.second << "\n";
	}
	outs() << "\n";
}

void dumpStore(const Store& store)
{
	outs() << "Store:\n";
	for (auto const& mapping: store)
	{
		outs() << "  " << *mapping.first << "  ->  " << mapping.second << "\n";
	}
	outs() << "\n";
}

void dumpTypeMap(const TypeMap& typeMap)
{
	outs() << "TypeMap:\n";
	for (auto const& mapping: typeMap)
	{
		outs() << "  " << *const_cast<Type*>(mapping.first) << "  ->  " << *mapping.second << "\n";
	}
	outs() << "\n";
}

}

void runAnalysisOnModule(const llvm::Module& module, const CommandLineOptions& opts)
{
	TypeAnalysis typeAnalysis;
	auto typeMap = typeAnalysis.runOnModule(module);

	PointerManager ptrManager;
	MemoryManager memManager;
	auto envStore = tpa::GlobalPointerAnalysis(ptrManager, memManager, typeMap).runOnModule(module);

	if (opts.shouldPrintType())
		dumpTypeMap(typeMap);

	dumpEnv(envStore.first);
	dumpStore(envStore.second);
}