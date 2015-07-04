#pragma once

namespace llvm
{
	class raw_ostream;
}

namespace annotation
{

class ExternalModRefTable;

class ExternalModRefTablePrinter
{
private:
	llvm::raw_ostream& os;
public:
	using TableType = ExternalModRefTable;

	ExternalModRefTablePrinter(llvm::raw_ostream& o): os(o) {}

	void printTable(const ExternalModRefTable&);
};

}
