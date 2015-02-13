
set (TPASourceCodes
	Analysis/TunableAliasAnalysis.cpp
	Analysis/TunablePointerAnalysis.cpp
	DataFlow/EvaluationWorkList.cpp
	DataFlow/PointerAnalysisEngine.cpp
	DataFlow/StaticCallGraph.cpp
	DataFlow/TransferFunction.cpp
	Pass/CFGBuildingPass.cpp
	Pass/DefUseGraphBuildingPass.cpp
	Pass/GlobalPointerAnalysisPass.cpp
	Pass/TunableAliasAnalysisPass.cpp
	Pass/TunableModRefAnalysisPass.cpp
	Pass/TunablePointerAnalysisPass.cpp
	Pass/TunableReachingDefAnalysisPass.cpp
)

add_library (TPA SHARED ${TPASourceCodes})
target_link_libraries (TPA PointerAnalysis)
target_link_libraries (TPA MemoryModel)