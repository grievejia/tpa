#include "PointerAnalysis/DataFlow/ModRefSummary.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

void ModRefSummaryMap::dump(raw_ostream& os) const
{
	os << "\n----- Mod-Ref Summary -----\n";
	for (auto const& mapping: summaryMap)
	{
		os << "Function " << mapping.first->getName() << '\n';
		
		os << "\tvalue reads : { ";
		for (auto val: mapping.second.value_reads())
			os << val->getName() << " ";
		os << "}\n\tmem reads : { ";
		for (auto loc: mapping.second.mem_reads())
			os << *loc << " ";
		os << "}\n\tmem writes : { ";
		for (auto loc: mapping.second.mem_writes())
			os << *loc << " ";
		os << "}\n";
	}
	os << "----- End of Summary -----\n";
}

}