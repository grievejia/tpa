#include "Transforms/LowerGlobalCtor.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <map>

using namespace llvm;

namespace transform
{

static bool lowerGlobalCtor(GlobalVariable& ctor, Module& module)
{
	auto mainFunc = module.getFunction("main");
	if (mainFunc == nullptr)
		return false;

	if (!ctor.hasUniqueInitializer())
    	return false;

    auto init = ctor.getInitializer();
    if (isa<ConstantAggregateZero>(init))
    	return false;
    auto initArray = cast<ConstantArray>(init);

    std::map<unsigned, Function*, std::greater<unsigned>> funMap;
    for (auto& elem: initArray->operands())
    {
    	auto elemVal = elem.get();
    	if (isa<ConstantAggregateZero>(elemVal))
    		continue;
    	auto cs = cast<ConstantStruct>(elemVal);
		if (isa<ConstantPointerNull>(cs->getOperand(1)))
			continue;
		if (!isa<Function>(cs->getOperand(1)))
			continue;

		auto prio = cast<ConstantInt>(cs->getOperand(0));
		funMap[prio->getZExtValue()] = cast<Function>(cs->getOperand(1));
	}

	auto insertPos = mainFunc->begin()->getFirstInsertionPt();
	for (auto const& mapping: funMap)
		CallInst::Create(mapping.second, {}, "", insertPos);

	ctor.eraseFromParent();
	return true;
}

static bool lowerGlobalDtor(GlobalVariable& dtor, Module& module)
{
	// FIXME: implement this
	return false;
}

bool LowerGlobalCtorPass::runOnModule(Module& module)
{
	bool modified = false;

	auto gctor = module.getGlobalVariable("llvm.global_ctors");
	if (gctor != nullptr)
		modified |= lowerGlobalCtor(*gctor, module);

	auto gdtor = module.getGlobalVariable("llvm.global_dtors");
	if (gdtor != nullptr)
		modified |= lowerGlobalDtor(*gdtor, module);

	return modified;
}

char LowerGlobalCtorPass::ID = 0;
static RegisterPass<LowerGlobalCtorPass> X("lower-global-ctor", "Turn global ctor into a function call at the start of main(), and global dtor into a function call at the end of main()", false, false);

}
