#include "PointerAnalysis/MemoryModel/MemoryObject.h"
#include "PointerAnalysis/MemoryModel/Pointer.h"
#include "PointerAnalysis/MemoryModel/Type/TypeLayout.h"
#include "PointerAnalysis/Support/PtsSet.h"
#include "Util/IO/PointerAnalysis/Printer.h"
#include "Util/Iterator/DereferenceIterator.h"
#include "Util/Iterator/InfixOutputIterator.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

using namespace llvm;
using namespace tpa;

namespace util
{
namespace io
{

void dumpValue(raw_ostream& os, const Value& value)
{
	if (isa<Function>(value))
		os << "$" << value.getName();
	else if (isa<GlobalValue>(value))
		os << "@" << value.getName();
	else if (isa<Constant>(value))
		os << "#" << value;
	else if (auto arg = dyn_cast<Argument>(&value))
		os << arg->getParent()->getName() << "/" << arg->getName();
	else if (auto inst = dyn_cast<Instruction>(&value))
		os << inst->getParent()->getParent()->getName() << "/" << inst->getName();
	else
		llvm_unreachable("Value print error");
}

raw_ostream& operator<< (raw_ostream& os, const Pointer& ptr)
{
	os << *ptr.getContext() << "::";
	dumpValue(os, *ptr.getValue());
	return os;
}

raw_ostream& operator<< (raw_ostream& os, const AllocSite& allocSite)
{
	switch (allocSite.getAllocType())
	{
		case AllocSiteTag::Null:
			os << "NULL SITE";
			break;
		case AllocSiteTag::Universal:
			os << "UNIV SITE";
			break;
		case AllocSiteTag::Global:
			os << "GLOBAL SITE ";
			dumpValue(os, *allocSite.getGlobalValue());
			break;
		case AllocSiteTag::Function:
			os << "FUNC SITE " << allocSite.getFunction()->getName();
			break;
		case AllocSiteTag::Stack:
			os << "STACK SITE " << *allocSite.getAllocContext() << ", ";
			dumpValue(os, *allocSite.getLocalValue());
			break;
		case AllocSiteTag::Heap:
			os << "HEAP SITE " << *allocSite.getAllocContext() << ", ";
			dumpValue(os, *allocSite.getLocalValue());
			break;
	}
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const MemoryObject& obj)
{
	auto const& allocSite = obj.getAllocSite();
	auto offset = obj.getOffset();
	os << "(";
	switch (allocSite.getAllocType())
	{
		case AllocSiteTag::Null:
			os << "nullobj";
			break;
		case AllocSiteTag::Universal:
			os << "anywhere";
			break;
		case AllocSiteTag::Global:
			dumpValue(os, *allocSite.getGlobalValue());
			if (offset > 0)
				os << " + " << offset;
			break;
		case AllocSiteTag::Function:
			assert(offset == 0);
			os << "$" << allocSite.getFunction()->getName();
			break;
		case AllocSiteTag::Stack:
			os << "S " << *allocSite.getAllocContext() << "::";
			dumpValue(os, *allocSite.getLocalValue());
			if (offset > 0)
				os << " + " << offset;
			break;
		case AllocSiteTag::Heap:
			os << "H " << *allocSite.getAllocContext() << "::";
			dumpValue(os, *allocSite.getLocalValue());
			if (offset > 0)
				os << " + " << offset;
			break;
	}
	if (obj.isSummaryObject())
		os << '\'';
	os << ')';
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const PtsSet& pSet)
{
	os << "{ ";
	std::copy(util::deref_const_iterator(pSet.begin()), util::deref_const_iterator(pSet.end()), infix_ostream_iterator<MemoryObject>(os, ", "));
	os << " }";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const tpa::ArrayTriple& triple)
{
	os << "(" << triple.start << ", " << triple.end << ", " << triple.size << ")";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const tpa::ArrayLayout& layout)
{
	os << "ArrayLayout {";
	std::copy(layout.begin(), layout.end(), infix_ostream_iterator<ArrayTriple>(os, ", "));
	os << "}";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const tpa::PointerLayout& layout)
{
	os << "PointerLayout {";
	std::copy(layout.begin(), layout.end(), infix_ostream_iterator<size_t>(os, ", "));
	os << "}";
	return os;
}

raw_ostream& operator<<(raw_ostream& os, const tpa::TypeLayout& type)
{
	os << "Type { size = " << type.getSize();
	if (!type.getArrayLayout()->empty())
		os << ", " << *type.getArrayLayout();
	if (!type.getPointerLayout()->empty())
		os << ", " << *type.getPointerLayout();
	os << " }";
	return os;
}


}
}