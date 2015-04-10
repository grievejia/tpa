#include "Client/Taintness/SourceSink/SinkSignature.h"
#include "Client/Taintness/SourceSink/SinkViolationClassifier.h"
#include "PointerAnalysis/DataFlow/DefUseModule.h"

#include <llvm/IR/BasicBlock.h>

using namespace llvm;
using namespace tpa;

namespace client
{
namespace taint
{

SinkViolationClassifier::SinkViolationClassifier(const tpa::DefUseModule& m): duModule(m)
{
}

void SinkViolationClassifier::addSinkSignature(const ProgramLocation& callsite, const SinkViolationRecords& records)
{
	auto inst = cast<Instruction>(callsite.getInstruction());
	auto func = inst->getParent()->getParent();

	auto& duFunc = duModule.getDefUseFunction(func);
	auto duInst = duFunc.getDefUseInstruction(inst);

	auto& existingRecords = recordsMap[ContextDefUseFunction(callsite.getContext(), &duFunc)];
	existingRecords.emplace_back(duInst, records);
}

}
}