#ifndef TPA_REACHING_DEF_MODULE_ANALYSIS_H
#define TPA_REACHING_DEF_MODULE_ANALYSIS_H

#include "PointerAnalysis/DataFlow/ReachingDefMap.h"

namespace llvm
{
	class Instruction;
	class Function;
}

namespace tpa
{

class ExternalModRefTable;
class PointerAnalysis;
class MemoryLocation;
class ModRefSummaryMap;

class ReachingDefModuleAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModRefTable& modRefTable;
public:
	ReachingDefModuleAnalysis(const PointerAnalysis& p, const ModRefSummaryMap& s, const ExternalModRefTable& t): ptrAnalysis(p), summaryMap(s), modRefTable(t) {}

	ReachingDefMap<llvm::Instruction> runOnFunction(const llvm::Function& func);
};

}

#endif
