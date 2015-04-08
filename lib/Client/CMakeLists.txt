
set (ClientSourceCodes
	Taintness/Analysis/TaintAnalysis.cpp
	Taintness/DataFlow/SourceSink.cpp
	Taintness/DataFlow/DefUseFunctionEvaluator.cpp
	Taintness/DataFlow/TaintAnalysisEngine.cpp
	Taintness/DataFlow/TaintPrecisionMonitor.cpp
	Taintness/DataFlow/TaintTransferFunction.cpp
	Taintness/Pass/TaintAnalysisPass.cpp
)

add_library (ClientsObj OBJECT ${ClientSourceCodes})
add_library (Clients SHARED $<TARGET_OBJECTS:ClientsObj>)
add_library (ClientsStatic STATIC $<TARGET_OBJECTS:ClientsObj>)

target_link_libraries (Clients TPA)
target_link_libraries (ClientsStatic TPAStatic)