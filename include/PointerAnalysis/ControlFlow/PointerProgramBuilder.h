#ifndef TPA_POINTER_PROGRAM_BUILDER_H
#define TPA_POINTER_PROGRAM_BUILDER_H

#include "PointerAnalysis/ControlFlow/PointerProgram.h"


namespace llvm
{
	class DataLayout;
}

namespace tpa
{

class PointerProgramBuilder
{
private:
	const llvm::DataLayout* dataLayout;

	void buildPointerCFG(PointerCFG& cfg);
	void constructDefUseChain(PointerCFG& cfg);
	PointerCFGNode* translateInstruction(PointerCFG&, const llvm::Instruction&);
	void computeNodePriority(PointerCFG& cfg);
public:
	PointerProgramBuilder(): dataLayout(nullptr) {}

	PointerProgram buildPointerProgram(const llvm::Module& module);
};

}

#endif
