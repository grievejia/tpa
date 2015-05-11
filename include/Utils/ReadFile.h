#ifndef TPA_UTIL_READ_FILE_H
#define TPA_UTIL_READ_FILE_H

#include <llvm/Support/MemoryBuffer.h>

#include <memory>

namespace tpa
{

std::unique_ptr<llvm::MemoryBuffer> readFileIntoBuffer(const std::string& fileName)
{
	auto fileOrErr = llvm::MemoryBuffer::getFile(fileName);
	if (auto ec = fileOrErr.getError())
	{
		auto errMsg = (llvm::Twine("Can't open file \'") + fileName + "\' :" + ec.message()).str();
		llvm::report_fatal_error(errMsg);
	}

	return std::move(fileOrErr.get());
}

}

#endif
