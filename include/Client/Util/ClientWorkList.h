#ifndef TPA_CLIENT_WORKLIST_H
#define TPA_CLIENT_WORKLIST_H

#include "Utils/EvaluationWorkList.h"

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class Module;
	class Function;
	class Instruction;
}

namespace client
{

class BasicBlockPrioComparator
{
private:
	llvm::DenseMap<const llvm::Instruction*, size_t> prioMap;
public:
	BasicBlockPrioComparator(const llvm::Module& module);

	size_t getPriority(const llvm::Instruction* bb) const;

	auto getComparator() const
	{
		return [this] (const llvm::Instruction* b0, const llvm::Instruction* b1)
		{
			return getPriority(b0) < getPriority(b1);
		};
	}
};

class ClientWorkList
{
private:
	using WorkListType = tpa::EvaluationWorkList<llvm::Function, llvm::Instruction, decltype(std::declval<BasicBlockPrioComparator>().getComparator())>;

	BasicBlockPrioComparator comp;
	WorkListType workList;
public:
	using LocalWorkList = WorkListType::LocalWorkList;

	ClientWorkList(const llvm::Module& module): comp(module), workList(comp.getComparator()) {}

	bool isEmpty() const
	{
		return workList.isEmpty();
	}

	void enqueue(const tpa::Context* ctx, const llvm::Function* f)
	{
		workList.enqueue(ctx, f);
	}

	void enqueue(const tpa::Context* ctx, const llvm::Function* f, const llvm::Instruction* b)
	{
		workList.enqueue(ctx, f, b);
	}

	auto dequeue()
	{
		return workList.dequeue();
	}

	LocalWorkList& getLocalWorkList(const tpa::Context* ctx, const llvm::Function* f)
	{
		return workList.getLocalWorkList(ctx, f);
	}
};

}

#endif
