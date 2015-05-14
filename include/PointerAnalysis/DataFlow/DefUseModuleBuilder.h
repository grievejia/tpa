#ifndef TPA_DEF_USE_MODULE_BUILDER_H
#define TPA_DEF_USE_MODULE_BUILDER_H

#include "PointerAnalysis/DataFlow/DefUseModule.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class ExternalModRefTable;
class ModRefSummaryMap;
class PointerAnalysis;

class DefUseModuleBuilder
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModRefTable& modRefTable;

	void buildDefUseFunction(DefUseFunction& f);
	//void buildTopLevelEdges(DefUseFunction& f);
	void buildMemLevelEdges(DefUseFunction& f);
public:
	DefUseModuleBuilder(const PointerAnalysis& p, const ModRefSummaryMap& m, const ExternalModRefTable& t): ptrAnalysis(p), summaryMap(m), modRefTable(t) {}
	DefUseModule buildDefUseModule(const llvm::Module& module);
};

}

#endif
