
set (PointerAnalysisSourceCodes
	Analysis/AliasAnalysis.cpp
	Analysis/GlobalPointerAnalysis.cpp
	Analysis/ModRefAnalysis.cpp
	Analysis/ModRefModuleAnalysis.cpp
	Analysis/PointerAnalysis.cpp
	Analysis/ReachingDefAnalysis.cpp
	Analysis/ReachingDefModuleAnalysis.cpp
	ControlFlow/PointerCFG.cpp
	ControlFlow/PointerProgramBuilder.cpp
	ControlFlow/SemiSparseProgramBuilder.cpp
	ControlFlow/StaticCallGraph.cpp
	ControlFlow/WriteDotFile.cpp
	DataFlow/DefUseGraph.cpp
	DataFlow/DefUseModule.cpp
	DataFlow/DefUseModuleBuilder.cpp
	DataFlow/DefUseProgramBuilder.cpp
	DataFlow/ModRefSummary.cpp
	DataFlow/WriteDotFile.cpp
	External/ExternalPointerEffectTable.cpp
	External/ExternalModTable.cpp
	External/ExternalRefTable.cpp
)
add_library (PointerAnalysis SHARED ${PointerAnalysisSourceCodes})
target_link_libraries (PointerAnalysis MemoryModel)