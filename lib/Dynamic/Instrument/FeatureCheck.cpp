#include "Dynamic/Instrument/FeatureCheck.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace dynamic
{

void FeatureCheck::issueWarning(const Value& v, const StringRef& msg)
{
	errs().changeColor(raw_ostream::RED);
	errs() << v.getName() << ": " << msg << "\n";
	errs().resetColor();
}

void FeatureCheck::checkIndirectLibraryCall(const Function& f)
{
	if (f.isDeclaration())
	{
		for (auto user: f.users())
		{
			ImmutableCallSite cs(user);
			if (!cs)
				continue;
			for (auto i = 0u; i < cs.arg_size(); ++i)
				if (cs.getArgument(i) == &f)
					issueWarning(f, "potential indirect external call found!");
		}
	}
}

void FeatureCheck::checkArrayArgOrInst(const Function& f)
{
	for (auto const& arg: f.args())
		if (arg.getType()->isArrayTy())
			issueWarning(arg, "param is an array");

	for (auto const& bb: f)
		for (auto const& inst: bb)
			if (inst.getType()->isArrayTy())
				issueWarning(inst, "inst is an array");
}

void FeatureCheck::runOnModule(const Module& module)
{
	for (auto const& f: module)
	{
		checkIndirectLibraryCall(f);
		checkArrayArgOrInst(f);
	}
}

}
