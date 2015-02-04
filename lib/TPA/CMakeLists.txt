
set (TPASourceCodes
	Analysis/TunableAliasAnalysis.cpp
	Analysis/TunablePointerAnalysis.cpp
	ControlFlow/PointerCFG.cpp
	ControlFlow/PointerProgramBuilder.cpp
	ControlFlow/SemiSparseProgramBuilder.cpp
	ControlFlow/WriteDotFile.cpp
	DataFlow/EvaluationWorkList.cpp
	DataFlow/ExternalPointerEffectTable.cpp
	DataFlow/PointerAnalysisEngine.cpp
	DataFlow/StaticCallGraph.cpp
	DataFlow/TransferFunction.cpp
	Pass/CFGBuildingPass.cpp
	Pass/GlobalPointerAnalysisPass.cpp
	Pass/TunableAliasAnalysisPass.cpp
	Pass/TunablePointerAnalysisPass.cpp
)

add_library (TPA SHARED ${TPASourceCodes})
target_link_libraries (TPA MemoryModel)