#pragma once

#include "Annotation/Taint/ExternalTaintTable.h"
#include "TaintAnalysis/Support/TaintEnv.h"
#include "TaintAnalysis/Support/TaintMemo.h"

namespace tpa
{
	class SemiSparsePointerAnalysis;
}

namespace taint
{

class DefUseModule;

class TaintAnalysis
{
private:
	TaintEnv env;
	TaintMemo memo;

	annotation::ExternalTaintTable extTable;
	const tpa::SemiSparsePointerAnalysis& ptrAnalysis;
public:
	TaintAnalysis(const tpa::SemiSparsePointerAnalysis& p): ptrAnalysis(p) {}

	void loadExternalTaintTable(const char* extFileName)
	{
		extTable = annotation::ExternalTaintTable::loadFromFile(extFileName);
	}

	bool runOnDefUseModule(const DefUseModule&);
};

}