
set (TPASourceCodes
	Analysis/TunablePointerAnalysis.cpp
	Analysis/TunablePointerAnalysisWrapper.cpp
	Analysis/TunableSparsePointerAnalysis.cpp
	Analysis/TunableSparsePointerAnalysisWrapper.cpp
	DataFlow/PointerAnalysisEngine.cpp
	DataFlow/PointerCFGEvaluator.cpp
	DataFlow/SparseAnalysisEngine.cpp
	DataFlow/TransferFunction.cpp
	Pass/CFGBuildingPass.cpp
	Pass/DefUseGraphBuildingPass.cpp
	Pass/DefUseModuleBuildingPass.cpp
	Pass/GlobalPointerAnalysisPass.cpp
	Pass/TunableAliasAnalysisPass.cpp
	Pass/TunableModRefAnalysisPass.cpp
	Pass/TunableModuleModRefAnalysisPass.cpp
	Pass/TunableModuleReachingDefAnalysisPass.cpp
	Pass/TunablePointerAnalysisPass.cpp
	Pass/TunableReachingDefAnalysisPass.cpp
	Pass/TunableSparsePointerAnalysis.cpp
)

add_library (TPA SHARED ${TPASourceCodes})
target_link_libraries (TPA PointerAnalysis)
target_link_libraries (TPA MemoryModel)