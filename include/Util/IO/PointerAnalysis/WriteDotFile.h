#pragma once

namespace tpa
{
	class CFG;
}

namespace util
{
namespace io
{

void writeDotFile(const char* filePath, const tpa::CFG& cfg);

}
}
