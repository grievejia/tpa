#ifndef TPA_EXTERNAL_TABLE_H
#define TPA_EXTERNAL_TABLE_H

#include <llvm/ADT/StringRef.h>

#include <vector>

namespace tpa
{

template <typename T>
struct EffectTrait
{
	// To be provided:
	// static T DefaultEffect;
	using UnknownEffect = typename T::UnknownEffectError;
};

template <typename T>
class ExternalTable
{
protected:
	using MappingType = std::pair<llvm::StringRef, T>;
	// Since this table is used in a strict query-after-insertion scenerio, we just need a sorted vector to hold all mappings
	std::vector<MappingType> table;

	void finalizeTable()
	{
		std::sort(table.begin(), table.end());
		assert(std::unique(table.begin(), table.end()) == table.end());
	}
public:
	explicit ExternalTable() {}

	T lookup(llvm::StringRef name) const
	{
		// Find the corresponding entry using binary search
		auto itr = std::lower_bound(table.begin(), table.end(), name,
			[] (const MappingType& entry, const llvm::StringRef& name)
			{
				return entry.first < name;
			});

		if (itr == table.end() || itr->first != name)
			return EffectTrait<T>::DefaultEffect;
		else
			return itr->second;
	}
};

}

#endif
