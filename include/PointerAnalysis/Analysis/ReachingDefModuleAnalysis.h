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

class ExternalModTable;
class PointerAnalysis;
class MemoryLocation;
class ModRefSummaryMap;

class ReachingDefModuleAnalysis
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModTable& extModTable;
public:
	ReachingDefModuleAnalysis(const PointerAnalysis& p, const ModRefSummaryMap& s, const ExternalModTable& t): ptrAnalysis(p), summaryMap(s), extModTable(t) {}

	ReachingDefMap<llvm::Instruction> runOnFunction(const llvm::Function& func);
};

}

#endif
