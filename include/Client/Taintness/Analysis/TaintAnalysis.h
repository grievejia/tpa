#ifndef TPA_TAINT_ANALYSIS_H
#define TPA_TAINT_ANALYSIS_H

namespace tpa
{
	class DefUseModule;
	class ExternalPointerEffectTable;
	class PointerAnalysis;
}

namespace client
{
namespace taint
{

class TaintAnalysis
{
private:
	const tpa::PointerAnalysis& ptrAnalysis;
	const tpa::ExternalPointerEffectTable& extTable;
public:
	TaintAnalysis(const tpa::PointerAnalysis& p, const tpa::ExternalPointerEffectTable& t): ptrAnalysis(p), extTable(t) {}

	// Return true if there is a info flow violation
	bool runOnDefUseModule(const tpa::DefUseModule& m, bool reportError = true);
};

}
}

#endif
