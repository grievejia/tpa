#ifndef TPA_DEF_USE_MODULE_BUILDER_H
#define TPA_DEF_USE_MODULE_BUILDER_H

#include "PointerAnalysis/DataFlow/DefUseModule.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class ExternalModTable;
class ExternalRefTable;
class ModRefSummaryMap;
class PointerAnalysis;

class DefUseModuleBuilder
{
private:
	const PointerAnalysis& ptrAnalysis;
	const ModRefSummaryMap& summaryMap;
	const ExternalModTable& extModTable;
	const ExternalRefTable& extRefTable;

	void buildDefUseFunction(DefUseFunction& f);
	//void buildTopLevelEdges(DefUseFunction& f);
	void buildMemLevelEdges(DefUseFunction& f);
public:
	DefUseModuleBuilder(const PointerAnalysis& p, const ModRefSummaryMap& m, const ExternalModTable& em, const ExternalRefTable& er): ptrAnalysis(p), summaryMap(m), extModTable(em), extRefTable(er) {}
	DefUseModule buildDefUseModule(const llvm::Module& module);
};

}

#endif
