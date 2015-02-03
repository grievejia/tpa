#ifndef TPA_PARSE_LLVM_ASSEMBLY_H
#define TPA_PARSE_LLVM_ASSEMBLY_H

#include <llvm/IR/Module.h>
#include <memory>

namespace tpa
{
std::unique_ptr<llvm::Module> parseAssembly(const char *assembly);
}

#endif
