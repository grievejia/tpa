#ifndef TPA_TAINT_PRECISION_LOSS_RECORD_H
#define TPA_TAINT_PRECISION_LOSS_RECORD_H

#include "Client/Taintness/Precision/PrecisionLossPositionSet.h"
#include "MemoryModel/Precision/ProgramLocation.h"

#include <unordered_map>

namespace client
{
namespace taint
{

class PrecisionLossRecord
{
private:
	using KeyType = tpa::ProgramLocation;
	using ValueType = PrecisionLossPositionSet<>;

	std::unordered_map<KeyType, ValueType> recordMap;
public:
	PrecisionLossRecord() = default;

	ValueType& getRecordAt(const KeyType& key)
	{
		return recordMap[key];
	}

	const ValueType* lookupRecord(const KeyType& key) const
	{
		auto itr = recordMap.find(key);
		if (itr == recordMap.end())
			return nullptr;
		else
			return &itr->second;
	}
};

}
}

#endif
