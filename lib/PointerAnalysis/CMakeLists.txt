
set (PointerAnalysisSourceCodes
	Analysis/GlobalPointerAnalysis.cpp
	Analysis/SemiSparsePointerAnalysis.cpp
	Context/AdaptiveContext.cpp
	Context/Context.cpp
	Context/KLimitContext.cpp
	Engine/ExternalCallAnalysis.cpp
	Engine/Initializer.cpp
	Engine/SemiSparsePropagator.cpp
	Engine/StorePruner.cpp
	Engine/TransferFunction.cpp
	Engine/Transfer/AllocTransfer.cpp
	Engine/Transfer/CallTransfer.cpp
	Engine/Transfer/CopyTransfer.cpp
	Engine/Transfer/EntryTransfer.cpp
	Engine/Transfer/LoadTransfer.cpp
	Engine/Transfer/OffsetTransfer.cpp
	Engine/Transfer/ReturnTransfer.cpp
	Engine/Transfer/StoreTransfer.cpp
	FrontEnd/ArrayLayoutAnalysis.cpp
	FrontEnd/CFGBuilder.cpp
	FrontEnd/CFGSimplifier.cpp
	FrontEnd/FunctionTranslator.cpp
	FrontEnd/InstructionTranslator.cpp
	FrontEnd/PointerLayoutAnalysis.cpp
	FrontEnd/PriorityAssigner.cpp
	FrontEnd/SemiSparseProgramBuilder.cpp
	FrontEnd/StructCastAnalysis.cpp
	FrontEnd/TypeAnalysis.cpp
	FrontEnd/TypeCollector.cpp
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
	Precision/PrecisionLossTracker.cpp
	Precision/ValueDependenceTracker.cpp
	Program/CFG.cpp
	Program/SemiSparseProgram.cpp
	Support/PtsSet.cpp
	StaticFields.cpp
)
add_library (PointerAnalysis STATIC ${PointerAnalysisSourceCodes})

target_link_libraries (PointerAnalysis Annotation LLVMSupport LLVMCore)