#include "PointerAnalysis/Engine/TransferFunction.h"
#include "Util/IO/PointerAnalysis/Printer.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace context;
using namespace llvm;

namespace tpa
{

void TransferFunction::addTopLevelSuccessors(const ProgramPoint& pp, EvalResult& evalResult)
{
	for (auto const succ: pp.getCFGNode()->uses())
		evalResult.addTopLevelProgramPoint(ProgramPoint(pp.getContext(), succ));
}

void TransferFunction::addMemLevelSuccessors(const ProgramPoint& pp, const Store& store, EvalResult& evalResult)
{
	for (auto const succ: pp.getCFGNode()->succs())
		evalResult.addMemLevelProgramPoint(ProgramPoint(pp.getContext(), succ), store);
}

EvalResult TransferFunction::eval(const ProgramPoint& pp)
{
	//errs() << "Evaluating " << pp.getCFGNode()->getFunction().getName() << "::" << pp << "\n";
	EvalResult evalResult;

	switch (pp.getCFGNode()->getNodeTag())
	{
		case CFGNodeTag::Entry:
		{
			evalEntryNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Alloc:
		{
			evalAllocNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Copy:
		{
			evalCopyNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Offset:
		{
			evalOffsetNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Load:
		{
			if (localState != nullptr)
				evalLoadNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Store:
		{
			if (localState != nullptr)
				evalStoreNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Call:
		{
			if (localState != nullptr)
				evalCallNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Ret:
		{
			if (localState != nullptr)
				evalReturnNode(pp, evalResult);
			break;
		}
	}

	return evalResult;
}

}
