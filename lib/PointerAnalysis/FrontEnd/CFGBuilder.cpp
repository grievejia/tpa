#include "PointerAnalysis/FrontEnd/CFG/CFGBuilder.h"
#include "PointerAnalysis/FrontEnd/CFG/CFGSimplifier.h"
#include "PointerAnalysis/FrontEnd/CFG/FunctionTranslator.h"
#include "PointerAnalysis/FrontEnd/CFG/InstructionTranslator.h"
#include "PointerAnalysis/Program/CFG/CFG.h"

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
	cfg.buildValueMap();
}

}