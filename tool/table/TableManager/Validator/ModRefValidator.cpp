#include "TableManager/Validator/TableValidator.h"
#include "PointerAnalysis/External/ModRef/ExternalModRefTable.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace tpa;

namespace table
{

namespace
{

SmallVector<uint8_t, 256> getIndices(const APosition& pos)
{
	SmallVector<uint8_t, 256> retVec;
	if (pos.isReturnPosition())
		retVec.push_back(0);
	else
	{
		auto const& argPos = pos.getAsArgPosition();
		if (argPos.isAfterArgPosition())
		{
			for (auto i = 1 + argPos.getArgIndex(), e = 256; i < e; ++i)
				retVec.push_back(i);
		}
		else
			retVec.push_back(1 + argPos.getArgIndex());
	}
	return retVec;
}

void validateEffect(const StringRef& funName, const ModRefEffect& effect, DenseSet<unsigned>& record)
{
	auto writeIdxs = getIndices(effect.getPosition());
	for (auto idx: writeIdxs)
	{
		if (record.count(idx))
			errs() << "Annotation for function " << funName << " is inconsistent\n";
		record.insert(idx);
	}
}

void validateSummary(const StringRef& funName, const ModRefEffectSummary& summary)
{
	DenseSet<unsigned> mods, refs;
	for (auto const& effect: summary)
	{
		if (effect.isModEffect())
			validateEffect(funName, effect, mods);
		else
			validateEffect(funName, effect, refs);
	}
}

}

void TableValidator<ExternalModRefTable>::validateTable(const ExternalModRefTable& table)
{
	for (auto const& mapping: table)
	{
		if (!mapping.second.isEmpty())
		{
			validateSummary(mapping.first, mapping.second);
		}
	}
}

}