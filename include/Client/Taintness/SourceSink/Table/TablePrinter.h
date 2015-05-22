#ifndef TPA_TAINT_TABLE_PRINTER_H
#define TPA_TAINT_TABLE_PRINTER_H

namespace llvm
{
	class raw_ostream;
}

namespace client
{
namespace taint
{

class ExternalTaintTable;

class TablePrinter
{
private:
	llvm::raw_ostream& os;
public:
	TablePrinter(llvm::raw_ostream&);

	void printTable(const ExternalTaintTable&) const;
};

}
}

#endif
