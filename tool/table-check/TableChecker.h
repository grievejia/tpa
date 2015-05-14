#ifndef TPA_TOOL_TABLECHECKER_H
#define TPA_TOOL_TABLECHECKER_H

namespace llvm
{
	class Module;
	class StringRef;
}

template <typename Table>
class TableChecker
{
private:
	Table table;
public:
	TableChecker(const llvm::StringRef& fileName);

	void check(const llvm::Module& module);
};

void checkPtrEffectTable(const llvm::Module&, const llvm::StringRef&);
void checkModRefEffectTable(const llvm::Module&, const llvm::StringRef&);
void checkTaintTable(const llvm::Module&, const llvm::StringRef&);

#endif
