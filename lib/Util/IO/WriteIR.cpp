#include "Util/IO/WriteIR.h"

#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/ToolOutputFile.h>

using namespace llvm;

namespace util
{
namespace io
{

void writeModuleToText(const Module& module, const char* fileName)
{
	std::error_code ec;
	tool_output_file out(fileName, ec, sys::fs::F_None);
	if (ec)
	{
		errs() << ec.message() << "\n";
		std::exit(-3);
	}

	module.print(out.os(), nullptr);

	out.keep();
}

void writeModuleToBitCode(const Module& module, const char* fileName)
{
	std::error_code ec;
	tool_output_file out(fileName, ec, sys::fs::F_None);
	if (ec)
	{
		errs() << ec.message() << "\n";
		std::exit(-3);
	}

	createBitcodeWriterPass(out.os())->runOnModule(const_cast<Module&>(module));

	out.keep();
}

void writeModuleToFile(const Module& module, const char* fileName, bool isText)
{
	if (isText)
		writeModuleToText(module, fileName);
	else
		writeModuleToBitCode(module, fileName);
}

}
}
