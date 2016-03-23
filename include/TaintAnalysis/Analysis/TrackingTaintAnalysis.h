#pragma once

#include "Annotation/Taint/ExternalTaintTable.h"
#include "TaintAnalysis/Support/ProgramPointSet.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintMemo.h"

namespace tpa
{
	class SemiSparsePointerAnalysis;
}

namespace taint
{

class DefUseModule;

class TrackingTaintAnalysis
{
private:
	TaintEnv env;
	TaintMemo memo;

	annotation::ExternalTaintTable extTable;
	const tpa::SemiSparsePointerAnalysis& ptrAnalysis;
public:
	TrackingTaintAnalysis(const tpa::SemiSparsePointerAnalysis& p): ptrAnalysis(p) {}

	void loadExternalTaintTable(const char* extFileName)
	{
		extTable = annotation::ExternalTaintTable::loadFromFile(extFileName);
	}

	std::pair<bool, ProgramPointSet> runOnDefUseModule(const DefUseModule&);
};

}