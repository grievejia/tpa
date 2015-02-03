include_directories (${tpa_SOURCE_DIR}/include/TPA)

set (TPASourceCodes
	Analysis/GlobalPointerAnalysis.cpp
	Analysis/TunableAliasAnalysis.cpp
	Analysis/TunablePointerAnalysis.cpp
	ControlFlow/PointerCFG.cpp
	ControlFlow/PointerProgramBuilder.cpp
	ControlFlow/SemiSparseProgramBuilder.cpp
	DataFlow/EvaluationWorkList.cpp
	DataFlow/ExternalPointerEffectTable.cpp
	DataFlow/PointerAnalysisEngine.cpp
	DataFlow/StaticCallGraph.cpp
	DataFlow/TransferFunction.cpp
	Memory/MemoryManager.cpp
	Memory/PointerManager.cpp
	Pass/CFGBuildingPass.cpp
	Pass/GlobalPointerAnalysisPass.cpp
	Pass/TunableAliasAnalysisPass.cpp
	Pass/TunablePointerAnalysisPass.cpp
	Precision/Context.cpp
	Precision/KLimitContext.cpp
	Utils/ParseLLVMAssembly.cpp
	Utils/PrettyPrint.cpp
	Utils/WriteDotFile.cpp
)
add_library (TPA SHARED ${TPASourceCodes})