#ifndef TPA_PARSE_LLVM_ASSEMBLY_H
#define TPA_PARSE_LLVM_ASSEMBLY_H

#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>

namespace tpa
{

// Mark the function inline so that the linker won't bark
inline std::unique_ptr<llvm::Module> parseAssembly(const char *assembly)
{
	auto module = std::make_unique<llvm::Module>("TestModule", llvm::getGlobalContext());

	llvm::SMDiagnostic error;
	bool parsed = llvm::ParseAssemblyString(assembly, module.get(), error, module->getContext()) == module.get();

	auto errMsg = std::string();
	llvm::raw_string_ostream os(errMsg);
	error.print("", os);

	if (!parsed)
	{
		// A failure here means that the test itself is buggy.
		llvm::report_fatal_error(os.str().c_str());
	}

	return std::move(module);
}

}

#endif
