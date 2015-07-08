#pragma once

namespace taint
{
	class DefUseFunction;
}

namespace util
{
namespace io
{

void writeDotFile(const char* filePath, const taint::DefUseFunction& duFunc);

}
}