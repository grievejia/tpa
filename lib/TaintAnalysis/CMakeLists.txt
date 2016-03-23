
set (TaintAnalysisSourceCodes
	Analysis/TrackingTaintAnalysis.cpp
	Analysis/TaintAnalysis.cpp
	Engine/ExternalCallAnalysis.cpp
	Engine/Initializer.cpp
	Engine/SinkViolationChecker.cpp
	Engine/TaintPropagator.cpp
	Engine/TransferFunction.cpp
	FrontEnd/DefUseModuleBuilder.cpp
	FrontEnd/ModRefModuleAnalysis.cpp
	FrontEnd/ReachingDefModuleAnalysis.cpp
	Output/TaintPrinter.cpp
	Output/WriteDotFile.cpp
	Precision/CallTracker.cpp
	Precision/PrecisionLossTracker.cpp
	Precision/ReturnTracker.cpp
	Precision/TrackerTransferFunction.cpp
	Program/DefUseModule.cpp
)
add_library (TaintAnalysis STATIC ${TaintAnalysisSourceCodes})

target_link_libraries (TaintAnalysis PointerAnalysis)