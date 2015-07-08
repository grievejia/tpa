
set (PointerAnalysisSourceCodes
	Analysis/GlobalPointerAnalysis.cpp
	Analysis/SemiSparsePointerAnalysis.cpp
	Program/CFG.cpp
	Program/SemiSparseProgram.cpp
	Context/AdaptiveContext.cpp
	Context/Context.cpp
	Context/KLimitContext.cpp
	Engine/ExternalCallAnalysis.cpp
	Engine/Initializer.cpp
	Engine/SemiSparsePropagator.cpp
	Engine/StorePruner.cpp
	Engine/TransferFunction.cpp
	FrontEnd/CFGBuilder.cpp
	FrontEnd/CFGSimplifier.cpp
	FrontEnd/FunctionTranslator.cpp
	FrontEnd/InstructionTranslator.cpp
	FrontEnd/PriorityAssigner.cpp
	FrontEnd/SemiSparseProgramBuilder.cpp
	FrontEnd/TypeAnalysis.cpp
	FrontEnd/TypeMap.cpp
	MemoryModel/ArrayLayout.cpp
	MemoryModel/MemoryManager.cpp
	MemoryModel/PointerLayout.cpp
	MemoryModel/PointerManager.cpp
	MemoryModel/TypeLayout.cpp
	Output/CFGNodePrinter.cpp
	Output/ContextPrinter.cpp
	Output/MemoryPrinter.cpp
	Output/NodePrinter.cpp
	Output/WriteDotFile.cpp
	Support/PtsSet.cpp
)
add_library (PointerAnalysis STATIC ${PointerAnalysisSourceCodes})

target_link_libraries (PointerAnalysis Annotation LLVMSupport LLVMCore)