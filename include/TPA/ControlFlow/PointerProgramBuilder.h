#ifndef TPA_POINTER_PROGRAM_BUILDER_H
#define TPA_POINTER_PROGRAM_BUILDER_H

#include "ControlFlow/PointerProgram.h"


namespace llvm
{
	class DataLayout;
}

namespace tpa
{

// The table is queried to exclude pointer unrelated external calls
class ExternalPointerEffectTable;

class PointerProgramBuilder
{
private:
	const llvm::DataLayout* dataLayout;
	const ExternalPointerEffectTable& extTable;

	void buildPointerCFG(PointerCFG& cfg);
	void constructDefUseChain(PointerCFG& cfg);
	PointerCFGNode* translateInstruction(PointerCFG&, const llvm::Instruction&);
	void computeNodePriority(PointerCFG& cfg);
public:
	PointerProgramBuilder(const ExternalPointerEffectTable& t): dataLayout(nullptr), extTable(t) {}

	PointerProgram buildPointerProgram(const llvm::Module& module);
};

}

#endif
