#ifndef TPA_MOD_REF_EFFECT_SUMMARY_H
#define TPA_MOD_REF_EFFECT_SUMMARY_H

#include "PointerAnalysis/External/ModRef/ModRefEffect.h"

#include <vector>

namespace tpa
{

class ModRefEffectSummary
{
private:
	using EffectList = std::vector<ModRefEffect>;
	EffectList list;
public:
	using const_iterator = EffectList::const_iterator;

	ModRefEffectSummary() = default;

	void addEffect(ModRefEffect&& e)
	{
		list.emplace_back(std::move(e));
	}

	const_iterator begin() const { return list.begin(); }
	const_iterator end() const { return list.end(); }
	bool isEmpty() const { return list.empty(); }
};

}

#endif
