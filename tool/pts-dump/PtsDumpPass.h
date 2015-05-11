#include <llvm/Pass.h>

class PtsDumpPass: public llvm::ModulePass
{
private:
public:
	static char ID;
	PtsDumpPass(): llvm::ModulePass(ID) {}

	bool runOnModule(llvm::Module& M) override;

	void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};