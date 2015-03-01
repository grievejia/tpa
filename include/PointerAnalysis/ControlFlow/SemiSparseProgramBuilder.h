#ifndef TPA_SEMI_SPARSE_PROGRAM_BUILDER_H
#define TPA_SEMI_SPARSE_PROGRAM_BUILDER_H

#include "PointerAnalysis/ControlFlow/PointerProgramBuilder.h"

namespace tpa
{

class ExternalPointerEffectTable;

class SemiSparseProgramBuilder
{
private:
	PointerProgramBuilder builder;

	void detachPreservingNodes(PointerCFG& cfg);
public:
	SemiSparseProgramBuilder(const ExternalPointerEffectTable& t): builder(t) {}

	PointerProgram buildSemiSparseProgram(const llvm::Module& module);
};

}

#endif