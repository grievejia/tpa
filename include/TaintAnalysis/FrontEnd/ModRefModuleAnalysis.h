#pragma once

#include "TaintAnalysis/FrontEnd/ModRefModuleSummary.h"

namespace llvm
{
	class Function;
	class Module;
}

namespace annotation
{
	class ExternalModRefTable;
}

namespace tpa
{
	class SemiSparsePointerAnalysis;
}

namespace taint
{

class ModRefModuleAnalysis
{
private:
	const tpa::SemiSparsePointerAnalysis& ptrAnalysis;
	const annotation::ExternalModRefTable& modRefTable;

	void collectProcedureSummary(const llvm::Function&, ModRefFunctionSummary&);
public:
	ModRefModuleAnalysis(const tpa::SemiSparsePointerAnalysis& p, const annotation::ExternalModRefTable& t): ptrAnalysis(p), modRefTable(t) {}

	ModRefModuleSummary runOnModule(const llvm::Module& module);
};

}
