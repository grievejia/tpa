#ifndef TPA_MOD_REF_MODULE_ANALYSIS_H
#define TPA_MOD_REF_MODULE_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ModRefSummary.h"

namespace tpa
{

class PointerAnalysis;
class ExternalModRefTable;

class ModRefModuleAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ExternalModRefTable& modRefTable;

	void collectProcedureSummary(const llvm::Function&, ModRefSummary&);
public:
	ModRefModuleAnalysis(const PointerAnalysis& p, const ExternalModRefTable& t): ptrAnalysis(p), modRefTable(t) {}

	ModRefSummaryMap runOnModule(const llvm::Module& module);
};

}

#endif
