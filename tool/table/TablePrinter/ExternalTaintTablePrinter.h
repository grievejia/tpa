#pragma once

namespace llvm
{
	class raw_ostream;
}

namespace annotation
{

class ExternalTaintTable;

class ExternalTaintTablePrinter
{
private:
	llvm::raw_ostream& os;
public:
	using TableType = ExternalTaintTable;

	ExternalTaintTablePrinter(llvm::raw_ostream& o): os(o) {}

	void printTable(const ExternalTaintTable&) const;
};

}
