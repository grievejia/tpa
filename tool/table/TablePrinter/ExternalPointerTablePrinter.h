#pragma once

namespace llvm
{
	class raw_ostream;
}

namespace annotation
{

class ExternalPointerTable;

class ExternalPointerTablePrinter
{
private:
	llvm::raw_ostream& os;
public:
	using TableType = ExternalPointerTable;

	ExternalPointerTablePrinter(llvm::raw_ostream& o): os(o) {}

	void printTable(const ExternalPointerTable&);
};

}