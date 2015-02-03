#include "Utils/ParseLLVMAssembly.h"
#include "Utils/STLUtils.h"

#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

std::unique_ptr<Module> parseAssembly(const char *assembly)
{
	auto module = std::make_unique<Module>("TestModule", getGlobalContext());

	SMDiagnostic error;
	bool parsed = ParseAssemblyString(assembly, module.get(), error, module->getContext()) == module.get();

	auto errMsg = std::string();
	raw_string_ostream os(errMsg);
	error.print("", os);

	if (!parsed) {
	  // A failure here means that the test itself is buggy.
	  report_fatal_error(os.str().c_str());
	}

	return std::move(module);
}

}
