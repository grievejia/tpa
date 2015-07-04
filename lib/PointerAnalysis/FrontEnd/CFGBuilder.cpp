#include "PointerAnalysis/FrontEnd/CFGBuilder.h"
#include "PointerAnalysis/FrontEnd/CFGSimplifier.h"
#include "PointerAnalysis/FrontEnd/FunctionTranslator.h"
#include "PointerAnalysis/FrontEnd/InstructionTranslator.h"

#include <llvm/IR/DataLayout.h>

using namespace llvm;

namespace tpa
{

CFGBuilder::CFGBuilder(CFG& c, const TypeMap& t): cfg(c), typeMap(t)
{

}

void CFGBuilder::buildCFG(const Function& llvmFunc)
{
	auto dataLayout = DataLayout(llvmFunc.getParent());
	auto instTranslator = InstructionTranslator(cfg, typeMap, dataLayout);
	FunctionTranslator(cfg, instTranslator).translateFunction(llvmFunc);
	CFGSimplifier().simplify(cfg);
}

}