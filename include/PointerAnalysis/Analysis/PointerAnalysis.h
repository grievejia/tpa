#ifndef TPA_POINTER_ANALYSIS_H
#define TPA_POINTER_ANALYSIS_H

#include "MemoryModel/PtsSet/PtsSetManager.h"

namespace llvm
{
	class Function;
	class Instruction;
	class Value;
}

namespace tpa
{

class Context;
class MemoryManager;
class Pointer;

class PointerAnalysis
{
public:
	virtual const MemoryManager& getMemoryManager() const = 0;
	virtual const PtsSet* getPtsSet(const llvm::Value* val) const = 0;
	virtual const PtsSet* getPtsSet(const Pointer* ptr) const = 0;
	virtual std::vector<const llvm::Function*> getCallTargets(const Context*, const llvm::Instruction*) const = 0;
	virtual ~PointerAnalysis() = default;
};

}

#endif
