
set (ClientSourceCodes
	Taintness/Analysis/PrecisionLossTrackingAnalysis.cpp
	Taintness/Analysis/TaintAnalysis.cpp
	Taintness/DataFlow/DefUseFunctionEvaluator.cpp
	Taintness/DataFlow/ExternalCallTaintTransferFunction.cpp
	Taintness/DataFlow/TaintAnalysisEngine.cpp
	Taintness/DataFlow/TaintTransferFunction.cpp
	Taintness/Pass/PrecisionLossTrackingPass.cpp
	Taintness/Pass/TaintAnalysisPass.cpp
	Taintness/Precision/DataFlowTracker.cpp
	Taintness/Precision/FunctionPrecisionLossTracker.cpp
	Taintness/Precision/InterProcDataFlowTracker.cpp
	Taintness/Precision/PrecisionLossTracker.cpp
	Taintness/Precision/TrackingTransferFunction.cpp
	Taintness/PrettyPrint.cpp
	Taintness/SourceSink/SinkViolationChecker.cpp
	Taintness/SourceSink/ExternalTaintTable.cpp
	Taintness/SourceSink/TablePrinter.cpp
)

add_library (ClientsObj OBJECT ${ClientSourceCodes})
add_library (Clients SHARED $<TARGET_OBJECTS:ClientsObj>)
add_library (ClientsStatic STATIC $<TARGET_OBJECTS:ClientsObj>)

target_link_libraries (Clients TPA)
target_link_libraries (ClientsStatic TPAStatic)