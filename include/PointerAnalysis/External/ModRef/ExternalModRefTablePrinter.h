#ifndef TPA_EXTERNAL_MOD_REF_TABLE_PRINTER_H
#define TPA_EXTERNAL_MOD_REF_TABLE_PRINTER_H

namespace llvm
{
	class raw_ostream;
}

namespace tpa
{

class ExternalModRefTable;

class ExternalModRefTablePrinter
{
private:
	llvm::raw_ostream& os;
public:
	ExternalModRefTablePrinter(llvm::raw_ostream& o): os(o) {}

	void printTable(const ExternalModRefTable&);
};

}

#endif
