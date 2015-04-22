#ifndef TPA_TAINT_ANALYSIS_H
#define TPA_TAINT_ANALYSIS_H

namespace tpa
{
	class DefUseModule;
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class TaintGlobalState;

class TaintAnalysis
{
private:
	const tpa::PointerAnalysis& ptrAnalysis;

	bool checkSinkViolation(const TaintGlobalState& globalState);
public:
	TaintAnalysis(const tpa::PointerAnalysis& p);

	// Return true if there is a info flow violation
	bool runOnDefUseModule(const tpa::DefUseModule& m);
};

}
}

#endif
