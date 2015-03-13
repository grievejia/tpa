
set (PointerAnalysisSourceCodes
	Analysis/AliasAnalysis.cpp
	Analysis/GlobalPointerAnalysis.cpp
	Analysis/ModRefAnalysis.cpp
	Analysis/PointerAnalysis.cpp
	Analysis/ReachingDefAnalysis.cpp
	ControlFlow/PointerCFG.cpp
	ControlFlow/PointerProgramBuilder.cpp
	ControlFlow/SemiSparseProgramBuilder.cpp
	ControlFlow/StaticCallGraph.cpp
	ControlFlow/WriteDotFile.cpp
	DataFlow/DefUseGraph.cpp
	DataFlow/DefUseProgramBuilder.cpp
	DataFlow/WriteDotFile.cpp
	External/ExternalPointerEffectTable.cpp
)
add_library (PointerAnalysis SHARED ${PointerAnalysisSourceCodes})
target_link_libraries (PointerAnalysis MemoryModel)