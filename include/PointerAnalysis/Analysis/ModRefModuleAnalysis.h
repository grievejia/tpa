#ifndef TPA_MOD_REF_MODULE_ANALYSIS_H
#define TPA_MOD_REF_MODULE_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ModRefSummary.h"

namespace tpa
{

class PointerAnalysis;
class ExternalModTable;
class ExternalRefTable;

class ModRefModuleAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ExternalModTable& extModTable;
	const ExternalRefTable& extRefTable;

	void collectProcedureSummary(const llvm::Function&, ModRefSummary&);
public:
	ModRefModuleAnalysis(const PointerAnalysis& p, const ExternalModTable& em, const ExternalRefTable& er): ptrAnalysis(p), extModTable(em), extRefTable(er) {}

	ModRefSummaryMap runOnModule(const llvm::Module& module);
};

}

#endif
