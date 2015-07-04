#include "PointerAnalysis/FrontEnd/TypeMap.h"

#include <cassert>

namespace tpa
{

void TypeMap::insert(const llvm::Type* key, const TypeLayout* val)
{
	typeMap[key] = val;
}

const TypeLayout* TypeMap::lookup(const llvm::Type* ty) const
{
	auto itr = typeMap.find(ty);
	if (itr == typeMap.end())
		return nullptr;
	else
	{
		assert(itr->second != nullptr);
		return itr->second;
	}
}

}